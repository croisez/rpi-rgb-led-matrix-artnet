# rpi-rgb-led-matrix-artnet
This is a very simple Art-Net gateway to 64x32 (or other sizes) RGB LED displays, using Raspberry Pi and the [rpi-rgb-led-matrix library](https://github.com/hzeller/rpi-rgb-led-matrix).

This is coded in C, and is supposed to be compiled and run on all kind of Raspberry Pi, though it has been tested on a Rpi3 only.

Note that only a small part of Art-Net protocol is implemented. It is only capable of receiving data coming from single-cast UDP datagrams. This gateway does not respond to requests.

I inspired myself from [darknessii project](https://github.com/darknessii/rpi-matrix-artnet), which was coded in Python. The choice of the C language was for performance reasons.

I am currently using [Jinx 2.4](http://www.live-leds.de/download/168/) as Art-Net client.
An example of configuration is given in this repository.
