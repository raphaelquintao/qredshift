# Changelog

### 1.0.0 - Wayland, Multi-Monitor, and More...

- **Added**
  * **Multi-Monitor Control:** Per-display temperature, brightness, and gamma targeting via `-d` flag across X11 and Wayland.
  * **Wayland Backend:** Support for `wlr-gamma-control-unstable-v1` compiled as a dynamically loaded (`dlopen()`) plugin—zero extra dependencies on pure X11 systems.
  * **Wayland Daemon (`-wd`):** FIFO IPC daemon mode with optional deferred initial value application.
  * **Developer Experience:** Added man page, bash completion script, and `3.0 (quilt)` compliant Debian packaging with Lintian support.

- **Changed / Improved**
  * **XCB Backend:** Fully rewritten to use a non-blocking, pipelined async architecture.
  * **Write Optimization:** No-op detection skips redundant gamma writes on all backends.
  * **Build System:** Restructured source tree with multi-arch `build/` / `bin/` output and configurable `PREFIX` rules.

- **Removed**
  * Dropped legacy architectures (`armv5tel`, `armv6l`, `mips`, `mipsel`, `s390x`).
 
### 0.13

* Add the `-interp` option to use the legacy Redshift-style interpolated gamma ramp calculation.
* Apply the selected gamma calculation mode consistently across both the XCB and Xlib backends.
* Embed the application name and version in the binary and bump the project version to `0.13`.
* Expanded the README with updated project background, feature details, and usage documentation.
* Limited input params to safe values.

### 0.12

* Uses XCB by default.
* Add `-xlib` flag to use Xlib instead of XCB for display manipulation.
* Improve `-i` output by reconstructing the active temperature, brightness, and gamma values from the current display gamma ramps.

### 0.11

* Added cross-compilation support using Docker.

### 0.10

* Initial release with basic functionality to adjust display temperature, brightness, and gamma.
