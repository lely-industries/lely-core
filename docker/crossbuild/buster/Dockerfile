FROM debian:buster

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
        libstdc++-8-dev:${ARCH} \
        qemu-user-static \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        cython \
        libpython-dev:${ARCH} \
        python \
        python-setuptools \
        python-wheel \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        cython3 \
        libpython3-dev:${ARCH} \
        python3 \
        python3-empy \
        python3-setuptools \
        python3-wheel \
        python3-yaml \
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

RUN git clone --branch v4.0 --depth 1 https://github.com/cpputest/cpputest.git \
    && cd cpputest/cpputest_build \
    && autoreconf .. -i \
    && ../configure --prefix=/usr/local/$NAME --host=$NAME \
    && make \
    && make install \
    && cd - \
    && rm -rf cpputest
