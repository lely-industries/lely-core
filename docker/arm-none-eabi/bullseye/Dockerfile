ARG REGISTRY

FROM ${REGISTRY}/build:bullseye

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        gcc-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib \
    && rm -rf /var/lib/apt/lists/*
