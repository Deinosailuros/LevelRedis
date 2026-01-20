FROM ubuntu:24.04 as builder

USER root

RUN apt update && \
    apt install -y build-essential make cmake



