FROM debian:bullseye

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        build-essential \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        libbluetooth-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        automake \
        libtool \
    && rm -rf /var/lib/apt/lists/*
