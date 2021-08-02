ARG REGISTRY

FROM ${REGISTRY}/build:bullseye

# overwrite default CppUTest with one with ASAN support enabled
RUN git clone --branch v4.0 --depth 1 https://github.com/cpputest/cpputest.git \
    && cd cpputest/cpputest_build \
    && autoreconf .. -i \
    && ../configure --disable-memory-leak-detection --enable-sanitize-address \
    && make \
    && make install \
    && cd - \
    && rm -rf cpputest
