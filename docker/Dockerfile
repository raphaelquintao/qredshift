FROM debian:13-slim

LABEL org.opencontainers.image.source="https://github.com/raphaelquintao/qredshift"
LABEL org.opencontainers.image.description="Base image to cross-build qredshift debian packages"
LABEL org.opencontainers.image.licenses=Apache-2.0

SHELL ["/bin/bash", "-c"]

ARG DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386 && \
    dpkg --add-architecture arm64 && \
    dpkg --add-architecture armhf && \
    dpkg --add-architecture ppc64el && \
    dpkg --add-architecture riscv64

RUN apt-get update && apt-get upgrade -y &&  \
    apt-get install -y --no-install-recommends \
    build-essential \
    crossbuild-essential-i386 \
    crossbuild-essential-arm64 \
    crossbuild-essential-armhf \
    crossbuild-essential-armel \
    crossbuild-essential-ppc64el \
    crossbuild-essential-riscv64 \
    bash-completion \
    pkg-config \
    debhelper \
    devscripts \
    dpkg-dev \
    git \
    sudo \
    lintian \
    libxrandr-dev:* libxcb1-dev:* libxcb-randr0-dev:* libwayland-dev:* \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash -u 1000 q &&  echo "q ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /home/q
USER q
CMD ["/bin/bash", "-l"]

