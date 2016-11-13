# DIY Position Tracking using HTC Vive's Lighthouse
 * General purpose indoor positioning sensor, good for robots, drones, etc.
 * 3d position accuracy: < 2 mm
 * Update frequency: 30 Hz
 * Output formats: Text; Mavlink ATT_POS_MOCAP via serial; Ublox GPS emulation (in works)
 * HTC Vive Station visibility requirements: full top hemisphere from sensor. Both stations need to be visible.
 * Positioning volume: same as HTC Vive, approx up to 4x4x3 meters.
 * Cost: ~$10 + [Teensy 3.2 ($20)](https://www.pjrc.com/store/teensy32.html) (+ [Lighthouse stations (2x $135)](http://www.vive.com/us/accessory/base-station/))
 * Skills to build: Low complexity soldering; Embedded C++ recommended for integration to your project.
 * License: MIT

| ![image](https://cloud.githubusercontent.com/assets/627997/19845384/a4fce0e4-9ef3-11e6-95e4-6567ff374ee0.png) | ![image](https://cloud.githubusercontent.com/assets/627997/19846322/79e76980-9efb-11e6-932e-7730e75dc5f1.png) |
| --- | --- |
| Demo showing raw XYZ position: [![image](https://cloud.githubusercontent.com/assets/627997/19845646/efc3cb18-9ef5-11e6-9902-58fe30e68a12.png)](https://www.youtube.com/watch?v=Xzuns5UYP8M) | Indoor hold position for a drone: [![image](https://cloud.githubusercontent.com/assets/627997/19845426/06c64bb2-9ef4-11e6-8d2b-1bbfbc5ec368.png)](https://www.youtube.com/watch?v=7GgB5qnx6_s) |

<!--
## Rationale
HTC Vive's Lighthouse is an amazing piece of technology that enables room-scale VR with precise 3D positioning.
-->

## How it works
Lighthouse position tracking system consists of:
 * two stationary infrared-emitting base stations (we'll use existing HTC Vive setup),
 * IR receiving sensor and processing module (this is what we'll create).

The base stations are usually placed high in the room corners and "overlook" the room.
Each station has an IR LED array and two rotating laser planes, horizontal and vertical.
Each cycle, after LED array flash (sync pulse), laser planes sweep the room horizontally/vertically with constant rotation speed.
This means that the time between the sync pulse and the laser plane "touching" sensor is proportional to horizontal/vertical angle
from base station's center direction to sensor.
Using the timing information, we can calculate 3d lines from each base station to sensor, the crossing of which yields
3d coordinates of our sensor.
Great thing about this approach is that it doesn't depend on light intensity and can be made very precise with cheap hardware.

Visualization of one base station (by rvdm88, click for full video):
[![How it works](http://i.giphy.com/ijMzXRF3OYBZ6.gif)](https://www.youtube.com/watch?v=oqPaaMR4kY4)

See also:
[This Is How Valve’s Amazing Lighthouse Tracking Technology Works – Gizmodo](http://gizmodo.com/this-is-how-valve-s-amazing-lighthouse-tracking-technol-1705356768)
[Lighthouse tracking examined – Oliver Kreylos' blog](http://doc-ok.org/?p=1478)

The sensor we're building is the receiving side of the Lighthouse. It will receive, recognize the IR pulses, calculate
the angles and produce 3d coordinates.

## How it works – details
Base stations are synchronized and work in tandem (they see each other's pulses). Each cycle only one laser plane sweeps the room,
so we fully update 3d position every 4 cycles (2 stations * horizontal/vertical sweep). Cycles are 8.333ms long, which is
exactly 120Hz. Laser plane rotation speed is exactly 180deg per cycle.

Each cycle, as received by sensor, has the following pulse structure:

| Pulse start, µs | Pulse length, µs | Source station | Meaning |
| --------: | ---------: | -------------: | :------ |
|         0 |     65–135 |              A | Sync pulse (LED array, omnidirectional) |
|       400 |     65-135 |              B | Sync pulse (LED array, omnidirectional) |
| 1222–6777 |        ~10 |         A or B | Laser plane sweep pulse (center=4000µs) |
|      8333 |            |                | End of cycle |

You can see all three pulses in the IR photodiode output (click for video):
[![Lighthouse pulse structure](https://cloud.githubusercontent.com/assets/627997/20243190/0e54fc44-a902-11e6-90cd-a4edf2464e7e.png)](https://youtu.be/7OFeN3gl3SQ)

The sync pulse lengths encode which of the 4 cycles we're receiving and station id/calibration data
(see [description](https://github.com/nairol/LighthouseRedox/blob/master/docs/Light%20Emissions.md)).

## Hardware


### Schematics
![schematics](https://ashtuchkin.github.io/vive-diy-position-sensor/sensor-schematics.svg)


## Software (Teensy)

### Installation instructions

Prerequisites:
 * [gcc-arm-embedded](https://launchpad.net/gcc-arm-embedded) toolchain. Can be installed on Mac with `brew cask install gcc-arm-embedded`.
   I'm developing with version 5_4-2016q3, but other versions should work too.
 * CMake 3.5+ `brew install cmake`
 * Command line uploader/monitor: [ty](https://github.com/Koromix/ty). See build instructions in the repo.
 * I recommend CLion as the IDE - it made my life a lot easier and can compile/upload right there.

Getting the code:
```bash
$ git clone https://github.com/ashtuchkin/vive-diy-position-sensor.git
$ cd vive-diy-position-sensor
$ git submodule update --init
```

Compilation/upload command line (example, using CMake out-of-source build in build/ directory):
```bash
$ cd build
$ cmake ..
$ make  # Build firmware
$ make vive-diy-position-sensor_Upload  # Upload to Teensy
$ tyc monitor  # Serial console to Teensy
```

## Internal notes
 * Update submodules to latest on branches/tags: `git submodule update --remote`
 * When changing .gitmodules, do `git submodule sync`
