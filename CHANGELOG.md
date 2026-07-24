# Changelog

* 1.0.0
  * Native multi-monitor support: target individual displays by index or
    RandR output name with per-display temperature, brightness, and gamma
    overrides via the -d flag. Works across all backends (XCB, Xlib,
    Wayland). One of the most requested features.
  * Add Wayland support via wlr-gamma-control-unstable-v1 protocol.
  * Compile Wayland backend as shared library (libqredshift_wayland_1.0.0.so),
    loaded at runtime via dlopen() only when a Wayland session is detected.
    X11 users do not need libwayland-client installed.
  * Implement Wayland daemon with FIFO-based IPC and new -wd flag to start
    the daemon without applying initial values.
  * Only update displays whose gamma ramps actually changed: skip writes when
    the current ramp matches the desired values, across both X11 and Wayland
    backends.
  * Completely rewrite XCB backend to a fully async pipelined architecture
    batching all output_info, gamma reads, and gamma writes in a single
    sync round-trip for maximum performance.
  * Add man page (qredshift.1) and Bash completion script, installed via
    make install.
  * Reorganize source tree.
  * Redesigned build system: per-architecture build/ and bin/ directories,
    separate compilation of main binary and Wayland plugin, install and
    uninstall targets with configurable PREFIX.
  * Improve Debian packaging:  proper source format (3.0 quilt), full
    Debian Policy-compliant packages, lintian integration.
  * Drop support for legacy architectures: armv5tel (armel), armv6l,
    mips/mipsel, and s390x.

* 0.13
  * Add the `-interp` option to use the legacy Redshift-style interpolated gamma ramp calculation.
  * Apply the selected gamma calculation mode consistently across both the XCB and Xlib backends.
  * Embed the application name and version in the binary and bump the project version to `0.13`.
  * Expanded the README with updated project background, feature details, and usage documentation.
  * Limited input params to safe values.
* 0.12
    * Uses XCB by default.
    * Add `-xlib` flag to use Xlib instead of XCB for display manipulation.
    * Improve `-i` output by reconstructing the active temperature, brightness, and gamma values from the current display gamma ramps.

* 0.11
    * Added cross-compilation support using Docker.
* 0.10
    * Initial release with basic functionality to adjust display temperature, brightness, and gamma.
