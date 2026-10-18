Bumblebee, a KillerBee-compatible firmware for TI CC2531
=========================================================

Introduction
------------

Bumblebee is a specific firmware designed for Texas Instruments CC2531 RF System-on-Chip that
allows IEEE 802.15.4 (ZigBee) packets sniffing and packet injection for RiverSecLoop's Killerbee
framework.

This firmware is based on Contiki (not Contiki-ng), and uses a USB CDC serial interface to communicate.

This firmware is designed to run on a $10 TI CC2531 USB device shown below:

![TI CC2531 usb dongle](https://github.com/virtualabs/cc2531-killerbee-fw/blob/main/images/ticc2531.webp "TI CC2531 Usb dongle")

Pre-compiled firmware
---------------------

The last compiled version of this firmware is available in [the Releases section](https://github.com/virtualabs/cc2531-killerbee-fw/releases). This is the recommended way to
get a fully working and tested version of Bumblebee.

Manual build
------------

First, you need to install the correct version of SDCC by [following these instructions](http://swannonline.co.uk/?q=node/60).
The patches in `patches/contiki/` are tested with SDCC 4.5.0; SDCC 4.0 and older need the
original `-Wp,-MMD,<dep>` form in `Makefile.customrules-cc253x`. See
[Reproducible build](#reproducible-build) for a build that needs no installed SDCC.

Then, clone this repository including the submodules:

```
$ git clone --recursive https://github.com/virtualabs/cc2531-killerbee-fw.git
```

Apply the Contiki patches and build the firmware:

```
$ cd cc2531-killerbee-fw
$ scripts/apply-contiki-patches.sh
$ make RELSTR=eabb1ce
```

The firmware file will be generated and named *cc2531-bumblebee.hex*.

### Contiki patches

The `contiki` submodule stays on contiki-os/contiki `eabb1ce`, which is archived.
The changes this firmware needs are in `patches/contiki/`, applied in order:

| patch | change |
|---|---|
| `0001-cc253x-build-with-sdcc-4.5.patch` | `--fsigned-char`, the sdcpp `-MF` dependency flag, two prototypes for SDCC 4.x |
| `0002-cc253x-usb-ep_tx-keep-loaded-in-fifo.patch` | `ep_tx()` leaves a loaded IN FIFO alone |
| `0003-cc253x-usb-reset-data-toggles.patch` | data toggles reset on SET_CONFIGURATION and ClearFeature(HALT) |

`scripts/apply-contiki-patches.sh` applies them to the submodule checkout; running it again is
a no-op. Contiki 3.0's cc253x code assumes that plain `char` is signed. SDCC 3.7 and later make
it unsigned, and the image then powers the USB pull-up but never answers EP0 (the host logs
`device descriptor read/64, error -110`). `--fsigned-char` fixes that, and the Makefile stops
if the patches are missing.

Contiki embeds `git describe` of `contiki/` (make variable `RELSTR`) in the image as its version
string, so pin `RELSTR` to get the same hex whatever way `contiki/` was checked out.

### Reproducible build

`scripts/build.sh WORKDIR [REV]` builds a commit (default `HEAD`) from `git archive`, with
Contiki fetched at the pinned commit and `patches/contiki/` applied. It uses Debian's SDCC 4.5.0
arm64 packages, fetched from snapshot.debian.org, checked by sha256 and extracted into `WORKDIR`;
nothing is installed. It prints the hex's sha256. On a host other than arm64, set `SDCC_BIN` to
a directory with SDCC; the hex may then differ.

How to install this firmware on a CC2531 USB dongle
---------------------------------------------------

If you own a *CC Debugger*, just [follow this tutorial](https://www.zigbee2mqtt.io/information/flashing_the_cc2531.html) and use Bumblebee firmware (.hex release) instead of ZNP.

If you are looking for alternatives, [follow this other tutorial](https://www.zigbee2mqtt.io/information/alternative_flashing_methods.html) with Bumblebee firmware. You may need an Arduino board, an ESP8266 or a Raspberry Pi to be able to flash your CC2531 USB dongle.

