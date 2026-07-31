# QRedshift

[![GitHub Release](https://img.shields.io/github/v/release/raphaelquintao/qredshift?include_prereleases&display_name=release&logo=github)](https://github.com/raphaelquintao/qredshift/releases/latest)
[![AUR Version](https://img.shields.io/aur/version/qredshift?style=flat&logo=archlinux&logoColor=white&logoSize=auto)](#arch-linux)

**QRedshift** is a stateless, high-performance, modern cross-display-server command-line utility written in C99 for adjusting screen color temperature, brightness, and gamma on Linux. Suporting both **X11** and **Wayland** with an ultra-lean **~40KB footprint**.

Designed for instant execution, qredshift operates entirely statelessly on X11, executing commands in a single shot without persistent background overhead. On Wayland setups, it includes a lightweight FIFO based daemon, explicitly required by the Wayland protocol to maintain active configuration state, ensuring compliant, near zero overhead operation. This architecture makes it highly optimized for desktop hotkeys, shell scripts, and automated display pipelines.

> **Component Disambiguation:** This repository contains the standalone C command-line tool (`qredshift`). If you are looking for the desktop panel widget for the Cinnamon desktop environment, refer to [QRedshiftCinnamon](https://github.com/raphaelquintao/QRedshiftCinnamon).

---

## Key Features

* **X11 & Wayland:** Seamless support for both X11 (via async XCB) and Wayland (via wlr-gamma-control-unstable-v1).
* **Native Multi-Monitor Control:** Adjust settings globally or target individual displays by name or index (`-d`) with per-display overrides.
* **Smart Gamma Ramp Skipping:** Queries current hardware state before execution; identical color targets are skipped to eliminate redundant updates.
* **Zero Overhead on X11:** Wayland backend is loaded dynamically at runtime via `dlopen()`. Pure X11 setups require no `libwayland-client` dependencies.
* **Stateless & Instant:** Operates with zero persistent background presence on X11, applying modifications instantly and exiting.
* **Wayland Daemon:** Features a minimal FIFO-based IPC daemon (`-wd`) necessary to satisfy Wayland's state-retention protocol requirements.
* **Minimal Footprint:** ~40KiB binary + ~50KiB for wayland plugin

## Why QRedshift?

Built with deep respect for **Redshift's** legacy, **QRedshift** is a modern C engine designed to bring Linux color management into the present era.

Modern desktop workflows with dynamic display hotplugging, multi-GPU setups, and hybrid X11/Wayland environments call for a modernized engine.

* **Rebuilding a Classic Foundation:** Replaces 15-year-old X11 code patterns with a modern, stateless architecture that requires zero configuration files or background processes on X11.
* **End to "[Hotplug Chaos](docs/ABOUT.md#the-hotplug-chaos)":** Replaces fragile positional array indexing with persistent hardware XIDs so display settings never swap when plugging into a dock or external screen.
* **Asynchronous XCB Engine:** Replaces 2010-era synchronous XCB calls with a modern asynchronous pipelined engine.
* **Named Output Control:** Seamlessly target individual monitors by name (`eDP-1`, `DP-2`) with custom per-display temperature and brightness curves.

📖 **[Deep dive into the architecture and X11 engine fixes in ABOUT.md](docs/ABOUT.md)**

## Installation


### Arch Linux

QRedshift is currently available on AUR repository:

```shell
yay -S qredshift
```

### Debian / Ubuntu

Download the `.deb` package from [latest release](https://github.com/raphaelquintao/qredshift/releases/latest) for your architecture:

To check your architecture use: `dpkg --print-architecture`
  
```shell
sudo dpkg -i qredshift_*.deb
```

### Generic Tarball
Download the `.tar.gz` package from [latest release](https://github.com/raphaelquintao/qredshift/releases/latest) for your architecture:

To check your architecture use: `uname -m`

```shell
tar -xzf qredshift_1.0.0_x86_64.tar.gz --one-top-level
cd qredshift_1.0.0_x86_64

# qredshift can run without any instalation
# if you dont use wayland you only need the binary `qredshift`
# if you use wayland make sure to keep `libqredshift_wayland_1.0.0.so` in the same folder of the binary
./qredshift -i 

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

<details>
   <summary><b>Xlib and XCB</b></summary>

* **Xlib** is the traditional, higher-level, and synchronous C library for interacting with the X Window System. It has been the standard for a long time but can be slower due to its synchronous nature.
* **XCB** (X C Binding) is a more modern, lower-level, and asynchronous replacement for Xlib. It allows for better performance by reducing round-trips to the X server and enabling parallel request processing.
* See: [The X New Developer's Guide: Xlib and XCB](https://www.x.org/wiki/guide/xlib-and-xcb/)

</details>


### Per-Display

`-t`, `-b`, and `-g` set global defaults. `-d` selects a specific display and optionally overrides those defaults for that display only. It can be repeated for multiple displays. Works on both X11 and Wayland.

Display can be identified by index or by name. Run `qredshift -i` to list available displays and their names.

## Examples

```terminaloutput
> qredshift -i

Display Server: x11
Desktop Environment: X-Cinnamon
Mode: XCB RandR
84:C:DisplayPort-0:1920x1080 | T: 6500K | B: 1.00 | G: 1.00
85:C:DisplayPort-1:1920x1080 | T: 6500K | B: 1.00 | G: 1.00
```

```shell
# apply to all displays
qredshift -t 4500 -b 0.9

# target one display by index (global params apply to it)
qredshift -t 4500 -d 84

# target one display by RandR name
qredshift -t 4500 -d DisplayPort-1

# per-display overrides - display 84 gets 6500K, display 85 gets 4500K at 80% brightness
qredshift -t 6500 -d 85:t=4500:b=0.8

# two displays, completely independent settings
qredshift -d DisplayPort-0:t=6500:b=1.0 -d 85:t=4500:b=0.8

# reset a single display
qredshift -d 85
```

Any parameter not specified in a `-d` block falls back to the global value. If no global is set either, the default applies.

### Wayland

On Wayland sessions, QRedshift runs as a lightweight background daemon. Start it with:

```shell
qredshift -wd      # start daemon without applying values
qredshift -t 4500  # start daemon and set temperature
```

```terminaloutput
> qredshift -i

Display Server: wayland
Desktop Environment: labwc:wlroots
Mode: Wayland (wlr-gamma-control-unstable-v1)
58:C:DP-1:1920x1080 | T: 6500K | B: 1.00 | G: 1.00
59:C:DP-2:1920x1080 | T: 6500K | B: 1.00 | G: 1.00
```

Subsequent invocations communicate through a FIFO pipe and return immediately.


## Compiling from Source

###### Debian

```shell
sudo apt-get install build-essential pkg-config libxrandr-dev libxcb1-dev libxcb-randr0-dev libwayland-dev
make
sudo make install
```

###### Arch

```shell
sudo pacman -Syu base-devel libxrandr libxcb xcb-util wayland
make
sudo make install
```

## Related Projects
 
- [QRedshiftCinnamon](https://github.com/raphaelquintao/QRedshiftCinnamon) — Cinnamon desktop applet using QRedshift as its backend


## Support the Project

I'm an independent developer, and projects like QRedshift are built in my free time because I genuinely enjoy creating software that solves problems in a simple and elegant way.

There is no company behind QRedshift just someone who enjoys writing software.

If you find the project useful and would like to help it continue to grow, consider becoming a sponsor.

Your support gives me more time to maintain existing projects, develop new features, and continue releasing open-source software.

And if you can't sponsor, don't worry.

A ⭐ on GitHub, a bug report, a feature suggestion, or simply recommending QRedshift to someone else is already a huge help.

Every contribution, no matter how small, helps keep the project alive.

Thank you.