cmake_minimum_required(VERSION 3.10)
project(LevelDBRedisModule C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 源文件
set(SOURCES
    src/leveldb_module.c
)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set(REDIS_GIT_URL https://github.com/redis/redis.git)
set(REDIS_GIT_TAG unstable)
set(REDIS_SOURCE_DIR ${PROJECT_ROOT}/deps/redis)

if(EXISTS ${REDIS_SOURCE_DIR}/.git)
    message(STATUS "Redis source exists, pulling latest changes...")
    execute_process(
        COMMAND git pull
        WORKING_DIRECTORY ${REDIS_SOURCE_DIR}
        RESULT_VARIABLE GIT_PULL_RESULT
    )
else()
    message(STATUS "Cloning Redis repository...")
    execute_process(
        COMMAND git clone --depth=1 -b ${REDIS_GIT_TAG} ${REDIS_GIT_URL} ${REDIS_SOURCE_DIR}
        RESULT_VARIABLE GIT_CLONE_RESULT
    )
endif()

include_directories("deps/redis/src/")

# LevelDB 头文件路径（修改为你本地路径）
include_directories("/usr/include")         # 假设 libleveldb-dev 安装在系统路径

# LevelDB 库路径（修改为你本地路径）
set(LEVELDB_LIB "/usr/lib/aarch64-linux-gnu/libleveldb.so")  # Linux 系统默认安装路径

# 生成共享库
add_library(leveldb_module SHARED ${SOURCES})

# 链接 LevelDB
target_link_libraries(leveldb_module ${LEVELDB_LIB})

