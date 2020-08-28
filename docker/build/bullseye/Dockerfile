FROM debian:bullseye

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        build-essential \
        ca-certificates \
        git \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        python3 \
        python3-empy \
        python3-setuptools \
        python3-wheel \
        python3-yaml \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        libbluetooth-dev \
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
    && ../configure --disable-memory-leak-detection \
    && make \
    && make install \
    && cd - \
    && rm -rf cpputest
