FROM debian:trixie-slim AS builder

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

RUN cmake --preset linux-release -DCMAKE_CXX_FLAGS=-O3 \
    && cmake --build --preset linux-release \
    && cmake --install out/build/linux-release --prefix /opt/linuxsoundscanner

FROM debian:trixie-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libglib2.0-0 \
    libpulse0 \
    libpulse-mainloop-glib0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/linuxsoundscanner/bin

COPY --from=builder /opt/linuxsoundscanner /opt/linuxsoundscanner

ENTRYPOINT ["/opt/linuxsoundscanner/bin/LinuxSoundScanner"]