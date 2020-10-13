ARG REGISTRY

FROM ${REGISTRY}/gcc:9

ARG COVERITY_PROJECT
ARG COVERITY_TOKEN

RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        ca-certificates \
        curl \
        wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://scan.coverity.com/download/linux64 \
        --post-data "token=$COVERITY_TOKEN&project=$COVERITY_PROJECT" \
        --no-verbose \
        -O coverity_tool.tgz \
    && wget https://scan.coverity.com/download/linux64 \
        --post-data "token=$COVERITY_TOKEN&project=$COVERITY_PROJECT&md5=1" \
        --no-verbose \
        -O coverity_tool.md5 \
    && echo " coverity_tool.tgz" >> coverity_tool.md5 \
    && md5sum -c coverity_tool.md5 \
    && rm -f coverity_tool.md5 \
    && tar xf coverity_tool.tgz -C /usr/local --strip-components 1 \
    && rm -f coverity_tool.tgz
