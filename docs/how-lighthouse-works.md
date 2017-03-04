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
