# MicroUI for APL: rxi's microui + built-in fenster renderer

This is a relatively faithful reimplementation of rxi's
[microui](https://github.com/rxi/microui) in pure APL. 
It provides an extremely lightweight and minimal immediate-mode GUI 
framework for writing cross-platform applications. 

Included is a Dyalog interface to [fenster](https://github.com/zserge/fenster),
a tiny and opinionated 2D Canvas/GUI library. This allows you to get up and 
running with microui without the need to write the rendering code. 

All of the code include demo, fenster, and the microui implementation, 
rendering code, and comments, comes in at ~1200 lines of code. 
The `resources` directory includes a 133KB atlas binary needed for the 
fenster render to render fonts and icons. This atlas was built using the 
[Inter](https://fonts.google.com/specimen/Inter) font. The fenster DLL
comes in around 100KB on my machine, and there are no other dependencies 
except for native platform libraries. 

We attempt to ensure that all of the same advantages found in the original 
library exist in this reimplementation. 

## Installation

MicroUI comes as a single `mu.apln' namespace that can be copied and loaded
into your APL session. 

You will need to either define your own rendering pipeline
(see `demo`, `begin_render`, `do_render`, and `end_render` for inspiration), 
or you can use the built-in fenster rendering. 
If you wish to use the fenster rendering, you'll need to compile 
`fenster.c` into a shared library and set `mu.fenster_dll` to the filename. 

	# Windows
	cl /LD fenster.c user32.lib gdi32.lib 
	# Linux
	cc -shared -o fenster.so fenster.c -lX11 -lasound

You should be able to then run the `mu.demo` and see everything working. 

## MicroUI API

See the microui [usage](https://github.com/rxi/microui/blob/master/doc/usage.md)
document for an overview of how microui works. 

See `mu.demo` and the code for how we translate the microui API into APL. 
Generally, controls that take options take the options as an optional left 
argument, and flag or option arguments, including return results, which are 
bitmasks in microui are flag bitvectors in APL, accessed using the appropriate
enumeration defined as a constant in `mu.apln`. 

The APIs are almost identical, except in some cases where you need to provide 
a static id for a control where the microui API relies on using the address of 
the data buffer as the static id.

We add one additional control for RGBA image matrices of the form 
`image <32-bit RGBA img_matrix>`. 

## Fenster API

See the `mu.begin_render`, `mu.do_render`, and `mu.end_render` functions for 
usage of the fenster API. We remain mostly compatible with the fenster API 
except that we do not provide explicit `fenster_pixel` macros, since APL 
already provides these well enough. Additionally, you must ensure that 
the framebuffer you pass to fenster is of 32-bit integer type.

## Known Issues

* Currently, closing a fenster window on Windows can cause the entire APL 
system to close down. This is inconsistent however. 
