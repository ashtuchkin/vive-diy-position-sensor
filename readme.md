# DIY Position Tracking using HTC Vive's Lighthouse and Teensy
 * 3d position accuracy: ~ 2 mm (std dev)
 * Update frequency: 30 Hz
 * Output formats: Text; Mavlink ATT_POS_MOCAP via serial; Ublox GPS emulation (in works)
 * Station visibility requirements: full top hemisphere from sensor. Both stations need to be visible.
 * Positioning volume: same as HTC Vive, approx up to 4x4x3 meters.
 * Cost: ~$10 + [Teensy 3.2 ($20)](https://www.pjrc.com/store/teensy32.html) (+ [Lighthouse stations (2x $135)](http://www.vive.com/us/accessory/base-station/))
 * Skills to build: Low complexity soldering; Embedded C++ recommended for integration to your project.
 * License: MIT

| ![image](https://cloud.githubusercontent.com/assets/627997/19845384/a4fce0e4-9ef3-11e6-95e4-6567ff374ee0.png) | ![image](https://cloud.githubusercontent.com/assets/627997/19846322/79e76980-9efb-11e6-932e-7730e75dc5f1.png) |
| --- | --- |
| Demo showing raw XYZ position: [![image](https://cloud.githubusercontent.com/assets/627997/19845646/efc3cb18-9ef5-11e6-9902-58fe30e68a12.png)](https://www.youtube.com/watch?v=Xzuns5UYP8M) | Sensor usage to stabilize drone: [![image](https://cloud.githubusercontent.com/assets/627997/19845426/06c64bb2-9ef4-11e6-8d2b-1bbfbc5ec368.png)](https://www.youtube.com/watch?v=7GgB5qnx6_s) |


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
