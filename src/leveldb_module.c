#include "redismodule.h"
#include <leveldb/c.h>
#include <stdlib.h>
#include <string.h>

// LevelDB 实例
static leveldb_t *db;
static leveldb_options_t *options;
static char *err = NULL;

// 打开数据库
int open_db(const char *path) {
    options = leveldb_options_create();
    leveldb_options_set_create_if_missing(options, 1);
    db = leveldb_open(options, path, &err);
    if (err != NULL) {
        return REDISMODULE_ERR;
    }
    return REDISMODULE_OK;
}

// leveldb.put key value
int LevelDBPut_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 3) return RedisModule_WrongArity(ctx);

    size_t key_len, val_len;
    const char *key = RedisModule_StringPtrLen(argv[1], &key_len);
    const char *val = RedisModule_StringPtrLen(argv[2], &val_len);

    leveldb_writeoptions_t *write_options = leveldb_writeoptions_create();
    leveldb_put(db, write_options, key, key_len, val, val_len, &err);
    leveldb_writeoptions_destroy(write_options);

    if (err != NULL) {
        RedisModule_ReplyWithError(ctx, err);
        free(err);
        err = NULL;
        return REDISMODULE_ERR;
    }

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

// leveldb.get key
int LevelDBGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) return RedisModule_WrongArity(ctx);

    size_t key_len;
    const char *key = RedisModule_StringPtrLen(argv[1], &key_len);

    leveldb_readoptions_t *read_options = leveldb_readoptions_create();
    size_t val_len;
    char *val = leveldb_get(db, read_options, key, key_len, &val_len, &err);
    leveldb_readoptions_destroy(read_options);

    if (err != NULL) {
        RedisModule_ReplyWithError(ctx, err);
        free(err);
        err = NULL;
        return REDISMODULE_ERR;
    }

    if (val == NULL) {
        RedisModule_ReplyWithNull(ctx);
    } else {
        RedisModule_ReplyWithStringBuffer(ctx, val, val_len);
        free(val);
    }

    return REDISMODULE_OK;
}

// leveldb.del key
int LevelDBDel_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) return RedisModule_WrongArity(ctx);

    size_t key_len;
    const char *key = RedisModule_StringPtrLen(argv[1], &key_len);

    leveldb_writeoptions_t *write_options = leveldb_writeoptions_create();
    leveldb_delete(db, write_options, key, key_len, &err);
    leveldb_writeoptions_destroy(write_options);

    if (err != NULL) {
        RedisModule_ReplyWithError(ctx, err);
        free(err);
        err = NULL;
        return REDISMODULE_ERR;
    }

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

int LevelDBBatch_RedisCommand(
    RedisModuleCtx *ctx,
    RedisModuleString **argv,
    int argc
) {
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    if (!db)
        return RedisModule_ReplyWithError(ctx, "ERR db not available");

    leveldb_writebatch_t *batch = leveldb_writebatch_create();

    int i = 1;
    while (i < argc) {
        size_t op_len;
        const char *op =
            RedisModule_StringPtrLen(argv[i], &op_len);

        if (strcasecmp(op, "put") == 0) {
            if (i + 2 >= argc) {
                leveldb_writebatch_destroy(batch);
                return RedisModule_ReplyWithError(
                    ctx, "ERR batch put needs key value");
            }

            size_t key_len, val_len;
            const char *key =
                RedisModule_StringPtrLen(argv[i + 1], &key_len);
            const char *val =
                RedisModule_StringPtrLen(argv[i + 2], &val_len);

            leveldb_writebatch_put(
                batch, key, key_len, val, val_len);

            i += 3;
        }
        else if (strcasecmp(op, "del") == 0) {
            if (i + 1 >= argc) {
                leveldb_writebatch_destroy(batch);
                return RedisModule_ReplyWithError(
                    ctx, "ERR batch del needs key");
            }

            size_t key_len;
            const char *key =
                RedisModule_StringPtrLen(argv[i + 1], &key_len);

            leveldb_writebatch_delete(batch, key, key_len);

            i += 2;
        }
        else {
            leveldb_writebatch_destroy(batch);
            return RedisModule_ReplyWithError(
                ctx, "ERR unknown batch operation");
        }
    }

    leveldb_writeoptions_t *wopt =
        leveldb_writeoptions_create();

    leveldb_write(db, wopt, batch, &err);

    leveldb_writeoptions_destroy(wopt);
    leveldb_writebatch_destroy(batch);

    if (err) {
        RedisModule_ReplyWithError(ctx, err);
        free(err);
        err = NULL;
        return REDISMODULE_ERR;
    }

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}


// RedisModule 初始化
int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (RedisModule_Init(ctx, "leveldb", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    char db_path[512];
    /* 1️⃣ 解析模块参数：数据库根目录 */
    if (argc >= 1) {
        size_t len;
        const char *path =
            RedisModule_StringPtrLen(argv[0], &len);

        if (len >= sizeof(db_path)) {
            RedisModule_Log(ctx, "warning",
                "DB path too long");
            return REDISMODULE_ERR;
        }

        memcpy(db_path, path, len);
        db_path[len] = '\0';
    } else {
        strcpy(db_path, "leveldb_data");
    }

    if (open_db(db_path) != REDISMODULE_OK) {
        RedisModule_Log(ctx, "warning", "Failed to open LevelDB");
        return REDISMODULE_ERR;
    }

    if (RedisModule_CreateCommand(ctx, "leveldb.put", LevelDBPut_RedisCommand, "write", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "leveldb.get", LevelDBGet_RedisCommand, "readonly", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "leveldb.del", LevelDBDel_RedisCommand, "write", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "leveldb.batch", LevelDBBatch_RedisCommand, "write", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    RedisModule_Log(ctx, "notice", "LevelDB module loaded!");
    return REDISMODULE_OK;
}

