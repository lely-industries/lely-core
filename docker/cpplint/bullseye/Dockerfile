FROM debian:bullseye

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        ca-certificates \
        git \
        python2 \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/google/styleguide.git \
    && cp styleguide/cpplint/cpplint.py /usr/local/bin \
    && rm -rf styleguide
