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

Then, clone this repository including the submodules:

```
$ git clone --recursive https://github.com/virtualabs/cc2531-killerbee-fw.git
```

And build the firmware:

```
$ cd cc2531-killerbee-fw
$ make
```

The firmware file will be generated and named *cc2531-bumblebee.hex*.

How to install this firmware on a CC2531 USB dongle
---------------------------------------------------

If you own a *CC Debugger*, just [follow this tutorial](https://www.zigbee2mqtt.io/information/flashing_the_cc2531.html) and use Bumblebee firmware (.hex release) instead of ZNP.

If you are looking for alternatives, [follow this other tutorial](https://www.zigbee2mqtt.io/information/alternative_flashing_methods.html) with Bumblebee firmware. You may need an Arduino board, an ESP8266 or a Raspberry Pi to be able to flash your CC2531 USB dongle.

