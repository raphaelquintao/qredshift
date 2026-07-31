# About QRedshift

QRedshift began in 2024 as a high-performance CLI for Linux display color management, powering the [QRedshiftCinnamon](https://github.com/raphaelquintao/QRedshiftCinnamon) applet (which dates back to 2019). It provides direct, stateless gamma ramp manipulation across both X11 and Wayland display environments.

## Why I Wrote QRedshift

I originally created **QRedshift Cinnamon** in 2019 as a personal project to improve my own desktop experience. After using it daily for a while, I decided to publish it.

To my surprise, it was embraced by the Cinnamon community, eventually becoming one of the most popular applets on Cinnamon Spices.

Over the years the applet grew far beyond a simple "Night Light" toggle, becoming a complete display temperature management tool with features **I personally missed on Linux**. As the project evolved, however, its original backend (redshift) became the limiting factor.

## Why Not Redshift?

Redshift served the Linux community exceptionally well for years, but its core architecture was built for a different era of Linux desktop infrastructure:

* **Archived Project:** Redshift was officially archived on April 1st, 2026.[^1]
* **Legacy Dependencies:** It relies on outdated components like GeoClue for location tracking, adding unnecessary complexity for users who only want direct color controls.
* **Daemon-Centric Model:** Redshift assumes a continuously running background daemon, which complicates external integration and leaves persistent background processes running on X11.
* **Unresolved X11 Engine Bugs:** Its core RandR and XCB drivers contain long-standing concurrency and monitor tracking bugs ([The Hotplug Chaos](#the-hotplug-chaos)) that were never modernized.

*(Other tools in the ecosystem address parts of this problem, but leave key gaps: **Gammastep** added Wayland support, but inherited Redshift's continuous daemon architecture and unedited X11 codebase unchanged. **sct / xsct** eliminated the daemon in favor of a tiny one-shot model, but lacks multi-monitor targeting, named display outputs, Wayland support, and gamma curve controls beyond basic brightness.)*

---

## Architecture & Core Features

QRedshift is **not** a fork of Redshift is an independent implementation built in from the ground up in pure POSIX-compliant C99 focused on direct gamma ramp manipulation without background overhead or mandatory configuration files.

### Stateless Execution (With Opt-In Wayland Support)

* **X11:** Operates strictly **one-shot**. It calculates gamma ramps, writes them directly to hardware registers, and exits immediately—leaving zero memory footprint.
* **Wayland:** Wayland protocols require gamma controls to stay tied to an active compositor connection. QRedshift handles this via an opt-in, lightweight background process connected over a FIFO pipe. The first call starts it, and subsequent calls execute instantly without re-establishing connections.

### Reverse Gamma Reconstruction

To support relative adjustments or status queries without keeping a daemon running or saving state files to disk, QRedshift uses a custom **reverse gamma ramp reconstruction algorithm**. It reads active hardware gamma ramps directly from the display server and back-calculates the original temperature (`-t`), brightness (`-b`), and gamma (`-g`) parameters on the fly.

### Multi-Monitor Targeting & Smart Delta Writes

* **Targeting:** Target up to 16 individual displays using `-d` by output name or index across XCB, Xlib, and Wayland backends.
* **Smart Delta Writes:** Reads active hardware ramps first and skips hardware writes if values already match the target, avoiding unnecessary GPU state changes during smooth transitions.
* **Algorithmic Ramps:** Computes gamma curves programmatically using Tanner Helland's [^3] color temperature research rather than relying on static precomputed lookup tables [^2] (though original lookup tables remain available via `-interp`).

---

## Critical X11 Engine Bugs Fixed

Beyond high-level architectural differences, QRedshift directly resolves core bugs in Redshift's underlying X11 codebase.

### The Hotplug Chaos

Legacy utilities like Redshift identify displays using raw list position numbers (e.g., monitor `0`, `1`, `2`). This assumes monitor `0` is always the exact same physical screen. On modern Linux setups, this assumption breaks regularly:

1. **Plugging In or Unplugging Displays:** Connecting an external monitor or docking a laptop forces the display server to rebuild its list. The new monitor can easily become index `0`, pushing your primary laptop screen to index `1`.
2. **Multi-GPU Boot Races:** Systems with dual GPUs (such as an integrated CPU graphics chip alongside a dedicated GPU) initialize drivers in unpredictable order at boot. If GPU 2 finishes initializing milliseconds ahead of GPU 1, internal display numbers shift.

When list indices shift, legacy tools apply your color profiles to the wrong screen, swap monitor settings, or fail entirely with `Configure crtc failed` errors.

**How QRedshift fixes it:** QRedshift discards list index numbers. Instead, it queries persistent hardware IDs (`RROutput` / `RRCrtc` XIDs) and display names (like `eDP-1` or `HDMI-A-1`). Color settings stay permanently bound to the correct physical monitor regardless of hotplugs, docking, or GPU startup order.

### Legacy Synchronous XCB Implementation

Dating back to the earliest releases of Redshift, the X11 engine was ported from Xlib to XCB by swapping function calls, but keeping Xlib’s old synchronous, blocking pattern. Major derivative tools inherited this code without inspecting how it worked under the hood.

Instead of taking advantage of XCB's asynchronous processing, legacy code issues serial, blocking request-reply round-trips for every display (calling `xcb_randr_get_crtc_gamma_reply` immediately after each request). On multi-monitor setups, the tool stalls on every display sequentially, multiplying latency.

**How QRedshift fixes it:** QRedshift implements a fully asynchronous, pipelined XCB engine as its primary X11 backend. It fires all display queries, fetches current gamma ramps, and pushes updated color curves in batched asynchronous pipelines with a single synchronization round-trip.

---

## Relationship to QRedshiftCinnamon

QRedshift is the standalone CLI engine responsible for low-level display control, temperature calculations, and backend management. [QRedshiftCinnamon](https://github.com/raphaelquintao/QRedshiftCinnamon) is the Cinnamon desktop applet that provides the graphical interface, system tray controls, and automated scheduling.

Both projects are maintained in separate repositories. Issues regarding UI, applet settings, or Cinnamon desktop integration belong in the applet repository, while issues regarding the CLI tool, gamma calculations, or display backends belong here.

[^1]: Original Redshift repository: <https://github.com/jonls/redshift>

[^2]: From Redshift: [README-colorramp](https://github.com/jonls/redshift/blob/master/README-colorramp)

[^3]: [How to Convert Temperature (K) to RGB: Algorithm and Sample Code](https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html)