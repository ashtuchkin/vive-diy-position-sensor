# HTC Vive Position Sensor using Teensy.



## Installation instructions

Prerequisites:
 * gcc-arm-embedded toolchain. Can be installed on Mac with `brew cask install gcc-arm-embedded`.
   I'm developing with version 5_4-2016q3, but other versions should work too.
 * CMake 3.5+ `brew install cmake`
 * Command line uploader/monitor: [ty](https://github.com/Koromix/ty). See build instructions in the repo.
 * I recommend CLion as the IDE - it made my life a lot easier.

Getting the code:
```bash
$ git clone https://github.com/ashtuchkin/vive-sensor-teensy.git
$ cd vive-sensor-teensy
$ git submodule update --init
```

Compilation/upload (example, using CMake out-of-source build in build/ directory):
```bash
$ cd build
$ cmake ..
$ make  # Build firmware
$ make vive-sensor-teensy_Upload  # Upload to Teensy
$ tyc monitor  # Serial console to Teensy
```




## Internal notes
 * Update submodules to latest on branches/tags: `git submodule update --remote`
 * When changing .gitmodules, do `git submodule sync`
