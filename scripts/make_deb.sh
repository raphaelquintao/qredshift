#!/bin/bash
set -e
# Copyright (c) 2024-2026 Raphael Quintao <raphaelquintao@gmail.com>
# SPDX-License-Identifier: Apache-2.0
#
# This script aims to generate a Debian Policy-compliant binary package.
#
# It probably does. I mean, it builds a package and lintian does not scream
# too loudly, so we are already ahead of many things in life.
#
# Something could be missing and sure be improved, since I got really bored
# and stop reading the "massive" Debian Policy Manual at some point.
#
# https://www.debian.org/doc/debian-policy/
# https://www.debian.org/doc/debian-policy/ch-binary.html

qecho() { printf "\e[%sm%b\e[0m" "$2" "$1"; }

PKG_NAME="${1}"
RAW_VERSION="${2}"
VERSION="${2}-1"
ARCH="${3:uname -m}"
DEB_DATE="${4:date -R}"
ITP_BUG_NUMBER="123456"

# Map incoming architecture string to the official Debian GNU architecture label
declare -A DEB_ARCH_MAP=(
  [x86_64]="amd64"
  [i686]="i386"
  [aarch64]="arm64"
  [armv7l]="armhf"
  [powerpc64le]="ppc64el"
)

DEB_ARCH="${DEB_ARCH_MAP[$ARCH]}"
if [ -z "$DEB_ARCH" ]; then
  qecho " => "
  qecho "Error: Unknown architecture format '$ARCH'.\n" "1;31"
  exit 1
fi

qecho "Building qredshift Version: $VERSION\n" "1;35"
qecho " => " "1"
qecho "GNU Arch: " "34"
qecho "$ARCH\n" "1"
qecho " => " "1"
qecho "DEB Arch: " "34"
qecho "$DEB_ARCH\n" "1"
qecho " => " "1"
qecho "Date: " "34"
qecho "Date: $DEB_DATE\n" "1"

PROJECT_ROOT="$(pwd)"
TMP_ROOT="$(mktemp -d /tmp/qredshift-deb-XXXXXX)"
trap 'rm -rf "$TMP_ROOT"' EXIT

WORKSPACE="$TMP_ROOT"

mkdir -p "$WORKSPACE"


# Copy Source
cp Makefile "$WORKSPACE/"
cp -R "$PROJECT_ROOT/src/" "$WORKSPACE/"
cp -R "$PROJECT_ROOT/data/" "$WORKSPACE/"


tar -cJf "/tmp/${PKG_NAME}_${RAW_VERSION}.orig.tar.xz" -C "$WORKSPACE" .


mkdir -p "$WORKSPACE/debian/source"
echo "3.0 (quilt)" > "$WORKSPACE/debian/source/format"


# ==========================================
# DYNAMIC DEBIAN METADATA GENERATION
# ==========================================
cat >"$WORKSPACE/debian/control" <<EOF
Source: $PKG_NAME
Section: utils
Priority: optional
Maintainer: Raphael Quintao <raphaelquintao@gmail.com>
Build-Depends: debhelper-compat (= 13), libx11-dev, libxrandr-dev, libxcb1-dev, libxcb-randr0-dev
Standards-Version: 4.7.0
Homepage: https://github.com/raphaelquintao/QRedshift

Package: $PKG_NAME
Architecture: amd64 i386 arm64 armhf ppc64el
Depends: \${shlibs:Depends}, \${misc:Depends}
Recommends: bash-completion
Description: Fast and Modern screen temperature tool supporting X11 multi-display and Wayland
 A command-line utility to manipulate screen color temperature based on
 user preferences. Designed to be lightweight and stateless, it features
 native multi-display support for X11 environments and compatibility with
 Wayland compositors implementing the wlr-roots protocol.
 .
 This tool serves as a modern replacement for legacy color temperature
 utilities like redshift.
EOF

cat >"$WORKSPACE/debian/changelog" <<EOF
$PKG_NAME ($VERSION) unstable; urgency=medium

  * Initial release. (Closes: #${ITP_BUG_NUMBER})

 -- Raphael Quintao <raphaelquintao@gmail.com>  $DEB_DATE
EOF

cat >"$WORKSPACE/debian/copyright" <<EOF
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Source: https://github.com/raphaelquintao/QRedshift
Upstream-Name: $PKG_NAME
Upstream-Contact: Raphael Quintao <raphaelquintao@gmail.com>

Files: *
Copyright: $(date +%Y) Raphael Quintao <raphaelquintao@gmail.com>
License: Apache-2.0

Files: debian/*
Copyright: $(date +%Y) Raphael Quintao <raphaelquintao@gmail.com>
License: Apache-2.0

License: Apache-2.0
 On Debian systems, the full text of the Apache License version 2
 can be found in the file "/usr/share/common-licenses/Apache-2.0".
EOF

#cat >"$WORKSPACE/debian/lintian-overrides" <<EOF
#$PKG_NAME: binary-from-other-architecture *
#EOF

cat >"$WORKSPACE/debian/rules" <<'EOF'
#!/usr/bin/make -f
include /usr/share/dpkg/architecture.mk

# Enable full compiler hardening (Stack protector, PIE, Fortify Source, etc.)
export DEB_BUILD_MAINT_OPTIONS = hardening=+all

%:
	dh $@

override_dh_auto_install:
	dh_auto_install -- "INSTALL=install --strip-program="

# Skip steps since there is no autoconf or test suite
override_dh_auto_configure:
override_dh_auto_test:

EOF

chmod +x "$WORKSPACE/debian/rules"


qecho "Starting compilation (-a $DEB_ARCH)\n" "1;35"

export DEB_HOST_ARCH="$DEB_ARCH"
export CFLAGS="$(dpkg-buildflags --get CFLAGS)"
export CXXFLAGS="$(dpkg-buildflags --get CXXFLAGS)"
export LDFLAGS="$(dpkg-buildflags --get LDFLAGS)"

cd "$WORKSPACE"
dpkg-buildpackage -us -uc -a "$DEB_ARCH" --check-command=lintian -Pcross


OUT_DIR="$PROJECT_ROOT/bin/$ARCH"
mkdir -p "$OUT_DIR"
OUT="${PKG_NAME}_${VERSION}_${DEB_ARCH}"

cp "bin/$ARCH/$PKG_NAME" "$OUT_DIR/"


cd ..
cp "$OUT.deb" "$OUT_DIR/"
#mv qredshift*.{deb,changes,buildinfo,dsc,tar.xz} "$OUT_DIR/"



#qecho "Lintian: $OUT_DEB\n" "1;35"
#lintian "$OUT_DIR/$OUT_DEB"
_NAME}_${VERSION}_${DEB_ARCH}".deb