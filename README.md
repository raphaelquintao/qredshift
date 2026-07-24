# QRedshift

QRedshift is a fast, stateless CLI tool for adjusting screen color temperature, brightness, and gamma on Linux. It reduces blue light and eye strain by shifting your display to warmer tones at night, and supports both X11 and Wayland compositors out of the box. Unlike traditional daemon-based tools, QRedshift operates in a one-shot stateless mode, applying changes instantly and exiting, with a ~40KB binary and zero runtime overhead. It features native multi-monitor per-display targeting, reverse gamma ramp reconstruction from the live display, and a Wayland plugin architecture that adds no dependency for X11 users. A modern replacement for redshift, sct, and gammastep.

Originally created as the backend for the [QRedshiftCinnamon](https://github.com/raphaelquintao/QRedshiftCinnamon) applet, it has since evolved into a standalone multiplatform CLI tool.

## Features

- [x] X11 support (XCB + Xlib backends)
- [x] Wayland support (wlr-gamma-control-unstable-v1)
- [x] Multi-monitor per-display targeting
- [x] Stateless one-shot operation (~40KB binary)
- [x] Zero Wayland dependency for X11 users (shared library plugin)
- [x] Reverse gamma ramp reconstruction from live display

## Planned

- [ ] DDC/CI brightness control for supported displays
- [ ] Native graphical interface (desktop-independent)

#### Long-term Goals

Although QRedshift began as the backend for the Cinnamon Applet, I'd like it to evolve into a standalone desktop application.

The goal is to build a lightweight, desktop-independent graphical interface that works consistently across Linux desktop environments, including Cinnamon, Xfce, MATE, GNOME, LXQt, and others.

Rather than relying on desktop-specific integrations or widget toolkits, the interface will be built as a unified native application with the same appearance and behavior everywhere.

The command-line utility will remain the project's foundation, while the graphical interface will simply become another frontend on top of it.

## Installation

### From Latest Release

Pre-built binaries and Debian packages are available on the [latest release](https://github.com/raphaelquintao/QRedshift/releases/latest) page:

- **Binary**: Download the `qredshift` executable for your architecture
- **Debian Package**: Download the `.deb` package for easy installation:
  ```shell
  sudo dpkg -i qredshift_*.deb
  ```

### Arch Linux (AUR)

A PKGBUILD is available in the `scripts/aur` directory. To build and install:

```shell
cd scripts
makepkg -si
```

### From Source

#### Build and Install

First, install the required dependencies, then compile and install:

###### Debian
```shell
sudo apt-get install libxrandr-dev libxcb1-dev libxcb-randr0-dev libwayland-dev
make
sudo make install
```

###### Arch
```shell
sudo pacman -Syu libxrandr libxcb xcb-util wayland
make
sudo make install
```

## Usage

Basic: `qredshift -t [temperature in Kelvin] -b [bright] -g [gamma]`

Reset: `qredshift -t`

| Parameter |                        | Description                      |
|-----------|------------------------|----------------------------------|
| `-h`      |                        | Display this help                |
| `-v`      |                        | Show program version             |
| `-i`      |                        | Show display info                |
| `-t`      | 6500                   | Temperature in kelvin            |
| `-b`      | 1.0                    | Brightness from 0.1 to 1.0       |
| `-g`      | 1.0                    | Gamma from 0.1 to 5.0            |
| `-d`      | `0:t=4500:b=1.0:g=1.0` | Target display (repeatable)      |
| `-xlib`   |                        | Use Xlib instead of XCB          |
| `-interp` |                        | Use legacy interpolation method  |
| `-wd`     |                        | Start Wayland daemon (no values) |

### Per-Display

`-t`, `-b`, and `-g` set global defaults. `-d` selects a specific display and optionally overrides those defaults for that display only. It can be repeated for multiple displays. Works on both X11 and Wayland.

Display can be identified by index or by RandR name. Run `qredshift -i` to list available displays and their names.

```shell
# apply to all displays
qredshift -t 4500 -b 0.9

# target one display by index (global params apply to it)
qredshift -t 4500 -d 1

# target one display by RandR name
qredshift -t 4500 -d HDMI-1

# per-display overrides - display 0 gets 6500K, display 1 gets 4500K at 80% brightness
qredshift -t 6500 -d 1:t=4500:b=0.8

# two displays, completely independent settings
qredshift -d 0:t=6500:b=1.0 -d 1:t=4500:b=0.8

# reset a single display
qredshift -d 1
```

Any parameter not specified in a `-d` block falls back to the global value. If no global is set either, the default applies.

### Wayland

On Wayland sessions, QRedshift runs as a lightweight background daemon. Start it with:

```shell
qredshift -wd      # start daemon without applying values
qredshift -t 4500  # start daemon and set temperature
```

Subsequent invocations communicate through a FIFO pipe and return immediately.

<details>
   <summary><b>Xlib and XCB</b></summary>

* **Xlib** is the traditional, higher-level, and synchronous C library for interacting with the X Window System. It has been the standard for a long time but can be slower due to its synchronous nature.
* **XCB** (X C Binding) is a more modern, lower-level, and asynchronous replacement for Xlib. It allows for better performance by reducing round-trips to the X server and enabling parallel request processing.
* See: [The X New Developer's Guide: Xlib and XCB](https://www.x.org/wiki/guide/xlib-and-xcb/)

</details>

## Compiling

###### Debian
```shell
sudo apt-get install libxrandr-dev libxcb1-dev libxcb-randr0-dev libwayland-dev
make
```

###### Arch

```shell
sudo pacman -Syu libxrandr libxcb xcb-util wayland
make
```

### Cross Compiling

###### Debian
```shell
sudo apt-get install docker.io 
sudo make docker
```

###### Arch

```shell
sudo pacman -Syu docker
sudo make docker
```

## Related

- [QRedshiftCinnamon](https://github.com/raphaelquintao/QRedshiftCinnamon) - Cinnamon applet that uses qredshift as its backend
- [docs/ABOUT.md](docs/ABOUT.md) - Project history, design philosophy, and comparison with Redshift

## Support the Project

I'm an independent developer, and projects like QRedshift are built in my free time because I genuinely enjoy creating software that solves problems in a simple and elegant way.

There is no company behind QRedshift just someone who enjoys writing software.

If you find the project useful and would like to help it continue to grow, consider becoming a sponsor.

Your support gives me more time to maintain existing projects, develop new features, and continue releasing open-source software.

And if you can't sponsor, don't worry.

A ⭐ on GitHub, a bug report, a feature suggestion, or simply recommending QRedshift to someone else is already a huge help.

Every contribution, no matter how small, helps keep the project alive.

Thank you.