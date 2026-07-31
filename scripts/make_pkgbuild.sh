#!/bin/bash
# Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
# https://github.com/raphaelquintao/qredshift
# SPDX-License-Identifier: Apache-2.0

set -e


VERSION="1.0.0"


PKGBUILD=$(cat << EOF
# Maintainer: Raphael Quintao <raphaelquintao@gmail.com>
pkgname=qredshift
pkgver=${VERSION}
pkgrel=1
pkgdesc="Stateless, modern multi-display screen color temperature CLI for X11 and Wayland"
arch=('x86_64' 'i686' 'aarch64' 'armv7h' 'ppc64le' 'riscv64')
url="https://github.com/raphaelquintao/qredshift"
license=('Apache-2.0')

depends=('glibc' 'libx11' 'libxrandr' 'libxcb' 'xcb-util')
optdepends=('wayland: Wayland compositor support via wlr-gamma-control')
provides=("\${pkgname}")
conflicts=("\${pkgname}")

source_x86_64=("https://github.com/raphaelquintao/qredshift/releases/download/v\$pkgver/\${pkgname}_\${pkgver}_x86_64.tar.gz")
source_i686=("https://github.com/raphaelquintao/qredshift/releases/download/v\$pkgver/\${pkgname}_\${pkgver}_i686.tar.gz")
source_aarch64=("https://github.com/raphaelquintao/qredshift/releases/download/v\$pkgver/\${pkgname}_\${pkgver}_aarch64.tar.gz")
source_armv7h=("https://github.com/raphaelquintao/qredshift/releases/download/v\$pkgver/\${pkgname}_\${pkgver}_armv7l.tar.gz")
source_ppc64le=("https://github.com/raphaelquintao/qredshift/releases/download/v\$pkgver/\${pkgname}_\${pkgver}_powerpc64le.tar.gz")
source_riscv64=("https://github.com/raphaelquintao/qredshift/releases/download/v\$pkgver/\${pkgname}_\${pkgver}_riscv64.tar.gz")

package() {
  cd "\$srcdir"

  install -Dm755 qredshift "\$pkgdir/usr/bin/qredshift"
  install -Dm755 "libqredshift_wayland_\$pkgver.so" "\$pkgdir/usr/lib/qredshift/libqredshift_wayland_\$pkgver.so"
  install -Dm644 bash_completion/qredshift "\$pkgdir/usr/share/bash-completion/completions/qredshift"
  install -Dm644 man/qredshift.1 "\$pkgdir/usr/share/man/man1/qredshift.1"
  install -Dm644 LICENSE.txt "\$pkgdir/usr/share/licenses/\$pkgname/LICENSE"
}
EOF
)

mkdir -p "aur"
cd aur
echo "$PKGBUILD" > PKGBUILD
updpkgsums
rm -f qredshift*.tar.gz
makepkg --printsrcinfo > .SRCINFO