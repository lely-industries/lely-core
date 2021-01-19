ARG REGISTRY

FROM ${REGISTRY}/build:bullseye

ARG SONAR_HOST_URL
ARG SONAR_TOKEN
ARG SONAR_SCANNER_VERSION=4.6.0.2311-linux

RUN apt-get update -q \
        && DEBIAN_FRONTEND=noninteractive apt-get install -qy --no-install-recommends \
        ca-certificates \
        curl \
        git \
        wget \
        unzip \
        python3 \
        python3-requests \
        && rm -rf /var/lib/apt/lists/*

# Download sonar-scanner
RUN curl -sSLo ./sonar-scanner.zip "https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-${SONAR_SCANNER_VERSION}.zip" \
        && unzip -o sonar-scanner.zip \
        && mv sonar-scanner-${SONAR_SCANNER_VERSION} /opt/sonar-scanner \
        && rm sonar-scanner.zip

# Download build-wrapper
RUN curl -sSLo ./build-wrapper-linux-x86.zip "${SONAR_HOST_URL}/static/cpp/build-wrapper-linux-x86.zip" \
        && unzip -oj build-wrapper-linux-x86.zip -d /opt/sonar-build-wrapper \
        && rm build-wrapper-linux-x86.zip
