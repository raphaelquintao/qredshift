# About QRedshift

QRedshift began as a custom backend for the
[QRedshiftCinnamon](https://github.com/raphaelquintao/QRedshiftCinnamon)
applet and evolved into a standalone multiplatform CLI tool. This page
documents the project's origins, design philosophy, and how it differs
from the original Redshift.

## Why I wrote QRedshift 

I originally created **QRedshift Cinnamon** in 2019 as a personal project to improve my own desktop experience. After using it daily for quite some time, I decided to publish it.

To my surprise, it was embraced by the Cinnamon community, eventually becoming one of the most popular applets on Cinnamon Spices.

Over the years the applet grew far beyond a simple "Night Light" toggle, becoming a complete display temperature management tool with features that **I personally missed on Linux**.

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

[1]: Original Redshift Repository: https://github.com/jonls/redshift

[2]: From Redshift: [README-colorramp](https://github.com/jonls/redshift/blob/master/README-colorramp)

[3]: [How to Convert Temperature (K) to RGB: Algorithm and Sample Code](https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html)