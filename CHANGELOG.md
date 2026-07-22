# Changelog

* 1.0
  * Streamline build pipeline and drop support for legacy architectures:
    - Drop armv5tel (armel), armv6l, and mips/mipsel because they lack
      support for modern GUI environments (GNOME/KDE Plasma) and are
      unsupported in upcoming Debian distributions.
    - Drop s390x as it is an enterprise IBM mainframe server architecture
      and does not run interactive X11 graphical desktop environments.

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