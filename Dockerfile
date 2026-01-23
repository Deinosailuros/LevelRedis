FROM ubuntu:24.04 AS builder

USER root

RUN apt update && \
    apt install -y build-essential make cmake

RUN apt install -y libleveldb-dev

COPY . /opt/builder/LevelRedis

RUN cd /opt/builder/LevelRedis && \
    mkdir -p build && \
    cd build && cmake .. && make

FROM ubuntu:24.04 as target

RUN apt update && \
    apt install -y libleveldb-dev redis-tools redis-server

RUN echo "loadmodule /opt/levelredis/lib/libleveldb_module.so /opt/levelredis/data/db" >> /etc/redis/redis.conf

RUN mkdir -p /opt/levelredis/etc

RUN sed 's/^bind 127.0.0.1.*/bind 0.0.0.0 -::*/' /etc/redis/redis.conf | \
    sed 's/protected-mode yes/protected-mode no/' | \
    sed 's/^daemonize yes/daemonize no/'> /opt/levelredis/etc/redis.conf

COPY --from=builder /opt/builder/LevelRedis/build/lib* /opt/levelredis/lib/

CMD redis-server /opt/levelredis/etc/redis.conf

