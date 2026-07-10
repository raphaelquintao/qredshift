# QRedshift CLI

A lightweight native Linux utility for adjusting display **color temperature**, **brightness**, and **gamma**. 

Originally developed as a Redshift replacement backend for the **QRedshift Cinnamon Applet**, it has since evolved into a standalone command-line application. 

### Cinnamon Applet

If you're looking for the Cinnamon applet, it has been moved to its own repository: [QRedshiftCinnamon](https://github.com/raphaelquintao/QRedshiftCinnamon).

## Why I wrote QRedshift 

I originally created **QRedshift Cinnamon** in 2019 as a personal project to improve my own desktop experience. After using it daily for quite some time, I decided to publish it.

To my surprise, it was embraced by the Cinnamon community, eventually becoming one of the most popular applets on Cinnamon Spices.

Over the years the applet grew far beyond a simple "Night Light" toggle, becoming a complete display temperature management tool with features that I personally missed on Linux.

As the project evolved, however, its original backend became the limiting factor.

### Why Not Just Use Redshift?

Redshift served the Linux community extremely well for many years, and this project would not exist without it.

However, over time its architecture diverged from the goals I had for QRedshift.

- The project had become effectively unmaintained.
- It was officially archived on April 1st, 2026. [^1]
- It still relied on legacy components such as GeoClue.
- Its architecture was built around a continuously running daemon.
- **The applet only required one thing**: change the display gamma ramps as quickly and reliably as possible.

Since the core functionality was relatively straightforward, I decided to write a dedicated backend instead of continuing to depend on Redshift.

## Differences from Redshift

QRedshift is **not** a fork of Redshift.

It is a completely independent implementation built around a different philosophy.

The goal isn't to replace every feature Redshift ever had.

The goal is to provide a lightweight, focused utility for applications that only need to manipulate display gamma.

### Stateless One-Shot Operation

Instead of running a permanent background daemon that continuously monitors time and manages state, QRedshift takes a stateless, **one-shot operation** approach.

When triggered, the application calculates the desired gamma ramps, applies them immediately to the X display, and exits instantly. This architecture makes the backend incredibly lightweight, blazing fast, and virtually dependency-free, relying only on native system libraries.

### Gamma Ramps Calculation

I also changed the default gamma ramps calculation method.

**Original Redshift** uses an interpolated RGB lookup table based on Mitchell Charity's work (revised by Ingo Thies) [^2].

**QRedshift** uses an algorithmic implementation based on the same research, using Tanner Helland's work as a starting point [^3]. To reduce approximation error, I tuned the polynomial coefficients to better match Redshift's interpolated reference. In practice, the resulting gamma curves are very close to the lookup-table approach for normal use.

This became the default implementation in QRedshift, while the original interpolation method remains available through the `-interp` option.


<details>
   <summary><b>Was this really necessary?</b></summary>

Probably not.

The lookup table is already fast, tiny, and perfectly adequate for modern hardware. The performance difference is effectively irrelevant.

I simply prefer an algorithmic solution when it can produce comparable results. It removes the dependency on precomputed tables and keeps the implementation more self-contained.

Sometimes isn't about making software noticeably faster. It's about making the implementation a little more elegant, even if nobody will ever notice.

</details>

### Reverse Gamma Reconstruction

Applying gamma ramps is relatively simple.

Recovering the original parameters is not.

Because QRedshift exits immediately after applying changes, there is no process left running that remembers the previous values.

If a user later requests a relative adjustment or simply wants to know the current settings, the application has no stored state to query.

Rather than introducing configuration files or a resident daemon, I chose a different approach.

QRedshift reads the active hardware gamma ramps directly from the X server and mathematically reconstructs the parameters that produced them.

A custom **reverse gamma ramp reconstruction algorithm** estimates the original `-t [temperature]`, `-b [brightness]`, and `-g [gamma]`, allowing QRedshift to determine its current operating state entirely from the live display.

This preserves the stateless architecture while enabling operations that would normally require persistent state.

## Support the Project

I'm an independent developer, and projects like QRedshift are built in my free time because I genuinely enjoy creating software that solves problems in a simple and elegant way.

There is no company behind QRedshift just someone who enjoys writing software.

If you find the project useful and would like to help it continue to grow, consider becoming a sponsor.

Your support gives me more time to maintain existing projects, develop new features, and continue releasing open-source software.

And if you can't sponsor, don't worry.

A ⭐ on GitHub, a bug report, a feature suggestion, or simply recommending QRedshift to someone else is already a huge help.

Every contribution, no matter how small, helps keep the project alive.

Thank you.


[^1]: Original Redshift Repository: https://github.com/jonls/redshift

[^2]: From Redshift: [README-colorramp](https://github.com/jonls/redshift/blob/master/README-colorramp)

[^3]: [How to Convert Temperature (K) to RGB: Algorithm and Sample Code](https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html)

## Features

- [x] Native X11 support
- [x] XCB backend (default)
- [x] Xlib backend
- [x] Tiny executable (~40KB)

## Planned

- [ ] Per-monitor configuration
- [ ] Wayland support (Not currently possible for Cinnamon/Muffin and Gnome/Mutter)
- [ ] DDC/CI brightness control for supported displays
- [ ] Native graphical interface

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
  
### From Source

#### Build and Install

First, install the required dependencies, then compile and install:

###### Debian
```shell
sudo apt-get install libxrandr-dev libxcb1-dev libxcb-randr0-dev
make
sudo make install
```

###### Arch
```shell
sudo pacman -Syu libxrandr libxcb xcb-util
make
sudo make install
```

## Usage

Basic: `qredshift -t [temperature in Kelvin] -b [bright] -g [gamma]`

Reset: `qredshift -t`

| Parameter | Description                     |
|-----------|---------------------------------|
| `-h`      | Display this help               |
| `-v`      | Show program version            |
| `-i`      | Show display info               |
| `-t` 6500 | Temperature in kelvin           |
| `-b` 1.0  | Brightness from 0.1 to 1.0      |
| `-g` 1.0  | Gamma from 0.1 to 1.0           |
| `-xlib`   | Use Xlib instead of XCB         |
| `-interp` | Use legacy interpolation method |

<details>
   <summary><b>Xlib and XCB</b></summary>

* **Xlib** is the traditional, higher-level, and synchronous C library for interacting with the X Window System. It has been the standard for a long time but can be slower due to its synchronous nature.
* **XCB** (X C Binding) is a more modern, lower-level, and asynchronous replacement for Xlib. It allows for better performance by reducing round-trips to the X server and enabling parallel request processing.
* See: [The X New Developer's Guide: Xlib and XCB](https://www.x.org/wiki/guide/xlib-and-xcb/)

</details>

## Compiling

###### Debian
```shell
sudo apt-get install libxrandr-dev libxcb1-dev libxcb-randr0-dev
make
```

###### Arch

```shell
sudo pacman -Syu libxrandr libxcb xcb-util
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

