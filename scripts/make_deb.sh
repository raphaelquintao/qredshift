#!/bin/bash
set -e
# Copyright (c) 2024-2026 Raphael Quintao <raphaelquintao@gmail.com>
# https://github.com/raphaelquintao/qredshift
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

BASE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

PKG_NAME="${1:-"qredshift"}"
RAW_VERSION="${2:-"0.0.0"}"
REVISION=${5:-1}
VERSION="${RAW_VERSION}-${REVISION}"
ARCH="${3:-$(uname -m)}"
DEB_DATE="${4:-$(date -R)}"

if [[ -z "${GPG_PRIVATE_KEY:-}" ]]; then
  echo "Error: GPG_PRIVATE_KEY environment variable is not set or empty."
  exit 1
else
  echo "GPG_PRIVATE_KEY found. Importing into GPG..."
  # Import the private key safely using echo and pipe
  echo -e "$GPG_PRIVATE_KEY" | gpg --batch --import
fi

# Map incoming architecture string to the official Debian GNU architecture label
declare -A DEB_ARCH_MAP=(
  [x86_64]="amd64"
  [i686]="i386"
  [aarch64]="arm64"
  [armv7l]="armhf"
  #  [armv5tel]="armel"
  #  [mips64el]="mips64el"
  #  [mipsel]="mipsel"
  [powerpc64le]="ppc64el"
  #  [s390x]="s390x"
  [riscv64]="riscv64"
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
qecho "$DEB_DATE\n" "1"

PROJECT_ROOT=$(dirname "$BASE")

#TMP_ROOT="$(mktemp -d /tmp/qredshift-deb-XXXXXX)"
TMP_ROOT="/tmp/qredshift-deb-$VERSION"
trap 'rm -rf "$TMP_ROOT"' EXIT

#TMP_ROOT="${PROJECT_ROOT}/TMP_ROOT"

rm -rf "${TMP_ROOT:?}/"*

WORKSPACE="$TMP_ROOT/workspace"
mkdir -p "$WORKSPACE/debian/source"

# Copy Source
cp "$PROJECT_ROOT/Makefile" "$WORKSPACE/"
cp -R "$PROJECT_ROOT/src/" "$WORKSPACE/"
cp -R "$PROJECT_ROOT/data/" "$WORKSPACE/"

cd "$WORKSPACE"

tar --exclude='debian' -cJf "../${PKG_NAME}_${RAW_VERSION}.orig.tar.xz" -- *

# ==========================================
# DYNAMIC DEBIAN METADATA GENERATION
# ==========================================
echo "3.0 (quilt)" >"$WORKSPACE/debian/source/format"

cat >"$WORKSPACE/debian/changelog" <<EOF
$PKG_NAME ($VERSION) unstable; urgency=medium

  * Initial release of standalone QRedshift. (Closes: #${ITP_BUG_NUMBER})
  * Added Wayland support via wlr-gamma-control-unstable-v1 protocol.
  * Added dynamic backend loading via dlopen().
  * Added native multi-monitor control (-d flag).
  * Upgraded packaging to source format 3.0 (quilt).

 -- Raphael Quintao <raphaelquintao@gmail.com>  $DEB_DATE
EOF

cat >"$WORKSPACE/debian/control" <<EOF
Source: ${PKG_NAME}
Section: utils
Priority: optional
Maintainer: Raphael Quintao <raphaelquintao@gmail.com>
Build-Depends: debhelper-compat (= 13), libwayland-dev, libx11-dev, libxrandr-dev, libxcb1-dev, libxcb-randr0-dev
Standards-Version: 4.7.0
Homepage: https://github.com/raphaelquintao/qredshift

Package: ${PKG_NAME}
Architecture: amd64 i386 arm64 armhf ppc64el riscv64
Depends: \${shlibs:Depends}, \${misc:Depends}
Recommends: bash-completion
Suggests: \${custom:WaylandDepends}
Description: Stateless, modern multi-display screen color temperature CLI for X11 and Wayland
 A modern, high-performance, stateless command-line utility for screen
 color temperature on Linux. Native multi-monitor for X11 and Wayland.
 .
 QRedshift lets you adjust screen temperature, brightness, and gamma
 correction on individual monitors, across X11 (XCB/Xlib) and Wayland
 (wlr-gamma-control-unstable-v1) display servers.
 .
 It replaces redshift, sct, and gammastep with a C99
 implementation that compiles to a ~40KB binary. Key features: native
 multi-monitor support, reverse gamma reconstruction without stored
 state, Wayland daemon mode with FIFO-based IPC, and a shared library
 plugin design that avoids pulling libwayland-client as a dependency
 on X11 systems.
EOF

cat >"$WORKSPACE/debian/copyright" <<EOF
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Source: https://github.com/raphaelquintao/qredshift
Upstream-Name: ${PKG_NAME}
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

cat >"$WORKSPACE/debian/rules" <<'EOF'
#!/usr/bin/make -f
include /usr/share/dpkg/architecture.mk

# Enable full compiler hardening (Stack protector, PIE, Fortify Source, etc.)
export DEB_BUILD_MAINT_OPTIONS = hardening=+all

%:
	dh $@

override_dh_gencontrol:
	# Parse the autogenerated substvars file
	@if [ -f debian/qredshift.substvars ]; then \
		WAYLAND_DEP=$$(grep -oE 'libwayland-client0[^,]*' debian/qredshift.substvars || true); \
		if [ -n "$$WAYLAND_DEP" ]; then \
			# Strip it out of the shlibs variable \
			sed -i -E "s/libwayland-client0[^,]*([, ]*)//g" debian/qredshift.substvars; \
			# Inject it safely as a custom variable \
			echo "custom:WaylandDepends=$$WAYLAND_DEP" >> debian/qredshift.substvars; \
		fi \
	fi
	# Execute the standard control generator passing the variables forward
	dh_gencontrol
	

override_dh_auto_install:
	dh_auto_install -- "INSTALL=install --strip-program="

# Skip steps since there is no autoconf or test suite
override_dh_auto_configure:
override_dh_auto_test:

EOF

chmod +x "$WORKSPACE/debian/rules"

qecho "Starting compilation (-a $DEB_ARCH)\n" "1;35"

#export ARCH
#echo $(dpkg-architecture -a"$DEB_ARCH")

#echo $ARCH
#echo $DEB_TARGET_GNU_CPU
#exit
#DEB_HOST_ARCH="$DEB_ARCH"
#CFLAGS="$(dpkg-buildflags --get CFLAGS)"
#CXXFLAGS="$(dpkg-buildflags --get CXXFLAGS)"
#LDFLAGS="$(dpkg-buildflags --get LDFLAGS)"

#export DEB_HOST_ARCH
#export CFLAGS CXXFLAGS LDFLAGS

#echo "$CFLAGS"
#exit

debuild --preserve-env -a"$DEB_ARCH" -k"raphaelquintao@gmail.com" -Pcross

PACKED="${PKG_NAME}_${RAW_VERSION}_${ARCH}.tar.gz"
#OUT_DEB="${PKG_NAME}_${VERSION}_${DEB_ARCH}.deb"


#BIN_OUT_DIR="$PROJECT_ROOT/bin/$ARCH"
#BUILD_OUT_DIR="$PROJECT_ROOT/build/$ARCH"
#mkdir -p "$BIN_OUT_DIR"
#mkdir -p "$BUILD_OUT_DIR"

#mkdir -p "$PROJECT_ROOT/bin/"

mkdir -p ../pack
cp "debian/$PKG_NAME/usr/bin"/* ../pack/
cp "debian/$PKG_NAME/usr/lib/$PKG_NAME"/* ../pack/
cp "$PROJECT_ROOT/LICENSE.txt" ../pack/
cp -R data/* ../pack/

DIST_DIR="$PROJECT_ROOT/dist/$ARCH"
mkdir -p "$DIST_DIR"

cd ..
cd pack
tar -czf "$PACKED" -- *
mv "$PACKED" "$DIST_DIR"


cd ..
mv qredshift*.{deb,changes,buildinfo,dsc,tar.xz} "$DIST_DIR"
