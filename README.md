# GF Amiga OpenCPN

OpenCPN 5.14.2, the chart plotter and navigation program, for AmigaOS 3 (68040; the FPU when there is one, through fpuauto), on the GFOpenWX Toolkit (wxWidgets on GFOpenLayout).

OpenCPN and what it builds on are fetched at build time at the commits `opencpn.lock` pins, and `patches/` is applied to OpenCPN; nothing third-party is kept in this repository.

## How it works

- OpenCPN's windows, menus, dialogs, tool bars and docking panes are wx, drawn by the GFOpenWX Toolkit.
- Charts are drawn with OpenCPN's own wxDC renderer: OpenGL is off in this build (`patches/0001`). OpenCPN draws raster (BSB/KAP), vector (S-57 ENC, CM93) charts and its world base map this way. MBTiles charts need OpenGL in OpenCPN, so they wait for the toolkit's wxGLCanvas.
- NMEA 0183 and NMEA 2000 come in over serial: `src/serial_amiga.cc` is the serial library on `serial.device` and the devices that speak its commands (USB serial adapters). Ports are listed from `DEVS:` and named `serial.device/0`.
- Sound (alarms) plays through wxSound, which the toolkit puts on AHI.
- NMEA 0183 and NMEA 2000 over TCP and UDP use the toolkit's sockets (bsdsocket.library).
- Not in this build yet (`patches/0005`, `src/net_parked.cpp`): Signal K, the REST server and sending routes to another OpenCPN, mDNS discovery, and downloads (chart catalogues), which need a websocket, HTTP server and HTTP client on bsdsocket; plugins (shared libraries elsewhere, `patches/0002`); Garmin host mode upload (`patches/0004`).

## Building

```
STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
```

The first run builds the toolkit's SDK, then fpuauto's library, then OpenCPN. The work tree is `~/.cache/gfamigaopencpn/build`, so a run stopped by a time limit carries on next time. `out/` gets `OpenCPN` and `OpenCPN-5.14.2.lha`, the drawer to install.

`scripts/Makefile.amiga` with `HOST=1` builds the same sources with the host's compiler and wxWidgets, a quick check of the patches (nothing of it is shipped).

| Where | What |
|---|---|
| `patches/` | OpenCPN's changes for the Amiga |
| `src/` | the serial.device driver, the GL names a GL-off build still links, and the parked network parts |
| `compat/include/` | OpenCPN's `config.h` for the Amiga, and `wx/glcanvas.h` with GL off |
| `scripts/` | the makefile, the source lists and the 68040 link specs |

## Licence

OpenCPN is GPL v2 or later; the patches and build files here are under the same licence. `src/serial_amiga.cc` and its header are MIT, as the serial library they plug into.

Designed, tested and delivered by The AmiCradle Team.
Copyright (c) 2026 AmiCradle Team
