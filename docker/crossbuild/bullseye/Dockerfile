FROM debian:bullseye

ARG ARCH

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        build-essential \
    && rm -rf /var/lib/apt/lists/*

RUN dpkg --add-architecture ${ARCH} \
    && apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        crossbuild-essential-${ARCH} \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        libstdc++-9-dev:${ARCH} \
        qemu-user-static \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        libbluetooth-dev:${ARCH} \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        automake \
        libtool \
    && rm -rf /var/lib/apt/lists/*
