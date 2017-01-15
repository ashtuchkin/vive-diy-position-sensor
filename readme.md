# DIY Position Tracking using HTC Vive's Lighthouse
 * General purpose indoor positioning sensor, good for robots, drones, etc.
 * 3d position accuracy: currently ~10mm; less than 2mm possible with additional work.
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

## How it works
Lighthouse position tracking system consists of:  
&nbsp;&nbsp;– two stationary infrared-emitting base stations (we'll use existing HTC Vive setup),  
&nbsp;&nbsp;– IR receiving sensor and processing module (this is what we'll create).  

The base stations are usually placed high in the room corners and "overlook" the room.
Each station has an IR LED array and two rotating laser planes, horizontal and vertical.
Each cycle, after LED array flash (sync pulse), laser planes sweep the room horizontally/vertically with constant rotation speed.
This means that the time between the sync pulse and the laser plane "touching" sensor is proportional to horizontal/vertical angle
from base station's center direction.
Using this timing information, we can calculate 3d lines from each base station to sensor, the crossing of which yields
3d coordinates of our sensor (see [calculation details](../../wiki/Position-calculation-in-detail)).
Great thing about this approach is that it doesn't depend on light intensity and can be made very precise with cheap hardware.

Visualization of one base station (by rvdm88, click for full video):  
[![How it works](http://i.giphy.com/ijMzXRF3OYBZ6.gif)](https://www.youtube.com/watch?v=oqPaaMR4kY4)

See also:  
[This Is How Valve’s Amazing Lighthouse Tracking Technology Works – Gizmodo](http://gizmodo.com/this-is-how-valve-s-amazing-lighthouse-tracking-technol-1705356768)  
[Lighthouse tracking examined – Oliver Kreylos' blog](http://doc-ok.org/?p=1478)  
[Reddit thread on Lighthouse](https://www.reddit.com/r/Vive/comments/40877n/vive_lighthouse_explained/)  

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
Complete tracking module consists of two parts:
 * IR Sensor and amplifier (custom board)
 * Timing & processing module (we use Teensy 3.2)

### IR Sensor
To detect the infrared pulses, of course we need an IR sensor. After a couple of attempts, I ended up using 
[BPV22NF](http://www.vishay.com/docs/81509/bpv22nf.pdf) photodiodes. Main reasons are:
 * Optical IR filter 790-1050nm, which excludes most of sunlight, but includes the 850nm stations use.
 * High sensitivity and speed
 * Wide 120 degree field of view

To get the whole top hemisphere FOV we need to place 3 photodiodes in 120deg formation in horizontal plane, then tilt each one
30deg in vertical plane. I used a small 3d-printed part, but it's not required. 

![image](https://cloud.githubusercontent.com/assets/627997/20243300/cf9b5e3a-a906-11e6-9137-3b0653bf694b.png)
![image](https://cloud.githubusercontent.com/assets/627997/20243325/b84964e2-a907-11e6-92bf-8b15d8c5cbd1.png)

### Sensor board

IR photodiodes produce very small current, so we need to amplify it before feeding to a processing module. I use 
[TLV2462IP](https://store.ti.com/TLV2462IP.aspx) opamp – a modern, general purpose
rail-to-rail opamp with good bandwidth, plus there are 2 of them in a chip, which is convenient.

One more thing we need to add is a simple high-pass filter to filter out background illumination level changes.

Full schematics:  
![schematics](https://ashtuchkin.github.io/vive-diy-position-sensor/sensor-schematics.svg)

| Top view | Bottom view |
| --- | --- |
| ![image](https://cloud.githubusercontent.com/assets/627997/20243575/291a2f7a-a913-11e6-9cd9-a152f66b2817.png) | ![image](https://cloud.githubusercontent.com/assets/627997/20243577/3d91bcb6-a913-11e6-9c58-30caf060dbc3.png) |

Part list (add to cart [from here](1-click-bom.tsv) using [1-click BOM](https://1clickbom.com)):

| Part | Model | Count | Cost (digikey) |
| --- | --- | --- | ---: |
| D1, D2, D3 | BPV22NF | 3 | 3x[$1.11](https://www.digikey.com/product-detail/en/vishay-semiconductor-opto-division/BPV22NF/751-1007-ND/1681141) |
| U1, U2 | TLV2462IP | 1 | [$2.80](https://www.digikey.com/product-detail/en/texas-instruments/TLV2462IP/296-1893-5-ND/277538) |
| Board | Perma-proto | 1 | [$2.95](https://www.digikey.com/product-detail/en/adafruit-industries-llc/1608/1528-1101-ND/5154676) |
| C1 | 5pF | 1 | [$0.28](https://www.digikey.com/product-detail/en/tdk-corporation/FG28C0G1H050CNT06/445-173467-1-ND/5812072) |
| C2 | 10nF | 1 | [$0.40](https://www.digikey.com/product-detail/en/tdk-corporation/FK24C0G1H103J/445-4750-ND/2050099) |
| R1 | 100k | 1 | [$0.10](https://www.digikey.com/product-detail/en/stackpole-electronics-inc/CF14JT100K/CF14JT100KCT-ND/1830399) |
| R2, R4 | 47k | 2 | 2x[$0.10](https://www.digikey.com/product-detail/en/stackpole-electronics-inc/CF14JT47K0/CF14JT47K0CT-ND/1830391) |
| R3 | 3k | 1 | [$0.10](https://www.digikey.com/product-detail/en/stackpole-electronics-inc/CF12JT3K00/CF12JT3K00CT-ND/1830498) |
| Total |  |  | $10.16 | 

Sample oscilloscope videos:

| Point | Video |
| --- | --- |
| After transimpedance amplifier: we've got a good signal, but notice how base level changes depending on background illumination (click for video) | [![first point](https://cloud.githubusercontent.com/assets/627997/20243649/83a717b2-a915-11e6-84d1-a4891baa33af.png)](https://youtu.be/PJGA8cOJhnc) |
| After high-pass filter: no more base level changes, but we see signal deformation | [![second point](https://cloud.githubusercontent.com/assets/627997/20243653/f532f6e4-a915-11e6-893a-b5964603dbc8.png)](https://youtu.be/ra8TT-KtqN0) |
| Sensor board output: 0-5v saturated signal | [![output](https://cloud.githubusercontent.com/assets/627997/20243669/b17aa0d6-a916-11e6-9f0a-7f499eebad14.png)](https://youtu.be/MdnZcimQteY)

### Teensy connection

| Teensy connections |  Full position tracker |
| --- | --- |
| ![image](https://cloud.githubusercontent.com/assets/627997/20243742/33e43b52-a919-11e6-9069-4cedc70f1c77.png) | ![image](https://cloud.githubusercontent.com/assets/627997/20243775/bca50ccc-a91a-11e6-8b45-33e086c21b3d.png) |

Note: Teensy's RX1/TX1 UART interface can be used to output position instead of USB. 


## Software (Teensy)

We use hardware comparator interrupt with ISR being called on both rise and fall edges of the signal. ISR (`cmp0_isr`) gets the timing 
in microseconds and processes the pulses depending on their lengths. We track the sync pulses lengths to determine which 
cycle corresponds to which base station and sweep. After the tracking is established, we convert time delays to angles and
calculate the 3d lines and 3d position (see geometry.cpp). After position is determined, we report it as text to USB console and
as Mavlink ATT_POS_MOCAP message to UART port (see mavlink.cpp).
 
NOTE: Currently, base station positions and direction matrices are hardcoded in geometry.cpp (`lightsources`). You'll need to 
adjust it for your setup. See [#2](//github.com/ashtuchkin/vive-diy-position-sensor/issues/2).

### Installation on macOS, Linux

Prerequisites:
 * [GNU ARM Embedded toolchain](https://launchpad.net/gcc-arm-embedded). Can be installed on Mac with `brew cask install gcc-arm-embedded`.
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

### Installation on Windows

I haven't been able to make it work in Visual Studio, so providing command line build solution.

Prerequisites:
 * [GNU ARM Embedded toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm). Windows 32 bit installer is fine.
 * [CMake 3.5+](https://cmake.org/download/). Make sure it's in your %PATH%.
 * [Ninja build tool](https://ninja-build.org/). Copy binary in any location in your %PATH%.

Getting the code is the same as above. GitHub client for Windows will make it even easier.

Building firmware:
```
cd build
cmake -G Ninja ..
ninja  # Build firmware. Will generate "vive-diy-position-sensor.hex" in current directory.
```
