# DIY Position Tracker using HTC Vive's Lighthouse  [![Build Status](https://travis-ci.org/ashtuchkin/vive-diy-position-sensor.svg?branch=master)](https://travis-ci.org/ashtuchkin/vive-diy-position-sensor)
Low cost, open source, DIY alternative to [Vive Tracker](https://www.vive.com/us/vive-tracker/). Put it on any object to get its 3d coordinates, quickly and precisely.
 * Can be connected to VR system, so you can see and interact with a physical object from the virtual reality world.
 * Or, you can use it as a general purpose indoor positioning sensor, no VR needed.
 * 3d positioning accuracy: ~1 mm at 30 Hz.
 * Positioning volume: same as HTC Vive, approx. up to 4x4x3 meters (a room). 
 * Output formats: Text, [Mavlink](https://en.wikipedia.org/wiki/MAVLink). Can be connected via USB, Serial port or (a bit later) Wifi.
 * Input sensor types: [TS3633-CM1](https://www.triadsemi.com/product/ts3633-cm1/), a DIY infrared sensor (see below) or HTC Vive's original sensors.
 * Multiple input sensors and multiple objects supported for a single processing board. 
 * Main processing board: [Teensy 3.1+](https://www.pjrc.com/teensy/) or [Particle Photon](https://www.particle.io/products/hardware/photon-wifi-dev-kit).
 * Note: Full HTC Vive room setup is currently required for the positioning to work. This requirement will be relaxed to having just the two base stations in the future.
 * License: MIT

## Demos
| Getting raw XYZ position | Drone position hold and precise landing
| --- | ---
| [![image](https://cloud.githubusercontent.com/assets/627997/19845646/efc3cb18-9ef5-11e6-9902-58fe30e68a12.png)](https://www.youtube.com/watch?v=Xzuns5UYP8M) | [![image](https://cloud.githubusercontent.com/assets/627997/19845426/06c64bb2-9ef4-11e6-8d2b-1bbfbc5ec368.png)](https://www.youtube.com/watch?v=7GgB5qnx6_s)

## High-level overview

..schema 

Base stations x2 -IR-> IR Sensor xN -Signal envelope-> Processing board -coordinates (usb/serial/wifi)-> VR System or Robot/Drone.

Complete tracking module consists of two parts:
 * IR Sensor and amplifier (custom board)
 * Timing & processing module (we use Teensy 3.2)


## Sensor types

All IR sensors listed below convert IR light from base stations to a raw digital signal.

| | TS3633-CM1 | "Lighty" DIY sensor | HTC Vive sensor 
| --- | --- | --- | ---
| Photo | [pic] | [pic] | [pic]
| Size | 12x10 mm | 51x43 mm | 20x6 mm
| Cost | $6.95 | ~$10 | not sold
| Pulse polarity | Negative | Positive | Positive
| Voltage | 3.3V | 5V | 3.3V
| Viewing Angle | 120° | 180° | 120°
| Signal/noise separation | Excellent | Average | Excellent 
| Wiring | 2.54mm holes | 2.54mm holes | 0.5mm FFC connector

TS3633-CM1 is recommended for all new designs as a small, stable and readily available sensor.

## Wiring

[schema]


## Main board setup and configuration

.. Upload provided .hex files to your board and configure it according to your setup. 

See [docs](docs/manual.md).


## Source code compilation

This is not needed for regular interaction with the tracker, but could be helpful for advanced configuration or functionality changes.

Prerequisites:
 * Install [GNU ARM Embedded toolchain, version 5-2016q3](https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update). Newer versions might work, but are not fully supported.
 * Install [CMake 3.2+](https://cmake.org/install/)
 * [Mac] Recommended Teensy command line uploader/monitor – [ty](https://github.com/Koromix/ty). See build instructions in the repo.
 * [Windows] Install [ninja build tool](https://ninja-build.org/). Copy binary into any location in your %PATH%.

Getting the code:
```bash
$ git clone https://github.com/ashtuchkin/vive-diy-position-sensor.git
$ cd vive-diy-position-sensor
$ git submodule update --init
```

[Mac, Linux] Building the code and uploading to Teensy:
```bash
$ cd build
$ cmake ..
$ make  # Build firmware
$ make vive-diy-position-sensor_Upload  # Upload to Teensy
$ tyc monitor  # Serial console to Teensy
```

[Windows] Building the code:
```bash
cd build
cmake -G Ninja ..
ninja  # Build firmware. Will generate "vive-diy-position-sensor.hex" in current directory.
```
To upload, use [Teensy loader](https://www.pjrc.com/teensy/loader.html)  
To interact with Teensy, use one of the terminal programs (see [overview from Sparkfun](https://learn.sparkfun.com/tutorials/terminal-basics/serial-terminal-overview-)).