FROM debian:bullseye

ARG ARCH

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        apt-transport-https \
        build-essential \
        curl \
        gnupg \
        software-properties-common \
        ca-certificates \
        git \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update -q \
    && curl -L https://dl.winehq.org/wine-builds/winehq.key | apt-key add - \
    && apt-add-repository 'deb https://dl.winehq.org/wine-builds/debian/ bullseye main' \
    && dpkg --add-architecture i386 \
    && apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        libc6:i386 \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        mingw-w64 \
        winehq-stable \
    && rm -rf /var/lib/apt/lists/*

ENV WINEPATH Z:\\usr\\lib\\gcc\\${ARCH}-w64-mingw32\\10-posix\;Z:\\usr\\${ARCH}-w64-mingw32\\lib

RUN wineboot -i

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        automake \
        libtool \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --branch v4.0 --depth 1 https://github.com/cpputest/cpputest.git \
    && cd cpputest/cpputest_build \
    && autoreconf .. -i \
    && ../configure --host=$ARCH-w64-mingw32 --disable-memory-leak-detection \
    && make \
    && make install \
    && cd - \
    && rm -rf cpputest
