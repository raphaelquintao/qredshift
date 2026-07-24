FROM debian:13-slim

WORKDIR /qredshift

ARG DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture amd64 && \
dpkg --add-architecture i386 && \
dpkg --add-architecture arm64 && \
dpkg --add-architecture armhf && \
dpkg --add-architecture ppc64el

RUN apt-get update && apt-get upgrade -y && apt-get install -y --no-install-recommends \
build-essential \
crossbuild-essential-i386 \
crossbuild-essential-arm64 \
crossbuild-essential-armhf \
crossbuild-essential-armel \
crossbuild-essential-ppc64el \
pkg-config \
debhelper \
dpkg-dev \
git \
lintian \
libxrandr-dev:* libxcb1-dev:* libxcb-randr0-dev:* libwayland-dev:* \
&& rm -rf /var/lib/apt/lists/*
