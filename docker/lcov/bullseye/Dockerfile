ARG REGISTRY

FROM ${REGISTRY}/build:bullseye

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        lcov \
    && rm -rf /var/lib/apt/lists/*
