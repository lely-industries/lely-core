ARG REGISTRY

FROM ${REGISTRY}/x86_64-w64-mingw32:bullseye

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        cppcheck \
    && rm -rf /var/lib/apt/lists/*
