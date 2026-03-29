# The builder image
FROM debian:trixie-slim AS builder

ARG PKG_VERSION=1.0.0

ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/opt/vcpkg

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git \
    libglib2.0-dev \
    libpulse-dev \
    ninja-build \
    pkg-config \
    tar \
    unzip \
    zip \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh"

WORKDIR /src

COPY . .

RUN cmake --preset linux-release -DPKG_VERSION="${PKG_VERSION}" \
    && cmake --build --preset linux-release \
    && cmake --install out/build/linux-release --prefix /opt/linuxsoundscanner

# The runtime image
FROM debian:trixie-slim AS runtime

ARG PKG_VERSION=1.0.0

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libglib2.0-0 \
    libpulse0 \
    libpulse-mainloop-glib0 \
    && rm -rf /var/lib/apt/lists/*

LABEL org.opencontainers.image.title="LinuxSoundScanner" \
      org.opencontainers.image.version="${PKG_VERSION}"

WORKDIR /opt/linuxsoundscanner/bin

COPY --from=builder /opt/linuxsoundscanner/bin/LinuxSoundScanner /opt/linuxsoundscanner/bin/LinuxSoundScanner
COPY --from=builder /opt/linuxsoundscanner/bin/LinuxSoundScanner.xml /opt/linuxsoundscanner/bin/LinuxSoundScanner.xml

ENTRYPOINT ["/opt/linuxsoundscanner/bin/LinuxSoundScanner"]