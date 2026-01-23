# LevelRedis
a redis module for leveldb

# Build with docker

```
docker build -t levelredis:v1 .
```

# start server

```
docker run --rm -it -p 6379:6379 -v $(pwd)/data:/opt/levelredis/data levelredis:v1
```

# use leveldb as redis

## PUT

```
redis-cli leveldb.put key value
```

## GET

```
redis-cli leveldb.get key
```

## DEL

```
redis-cli leveldb.del key
```

## BATCH

```
redis-cli leveldb.batch PUT key1 value1 PUT key2 value2 DEL key1 value1
```

