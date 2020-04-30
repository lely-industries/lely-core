FROM debian:bullseye

ARG ARCH
ARG NAME

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        build-essential \
        ca-certificates \
        git \
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
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 https://github.com/cpputest/cpputest.git \
    && cd cpputest/cpputest_build \
    && autoreconf .. -i \
    && ../configure --prefix=/usr/local/$NAME --host=$NAME \
    && make \
    && make install \
    && cd - \
    && rm -rf cpputest
