
| ![image](https://cloud.githubusercontent.com/assets/627997/19845384/a4fce0e4-9ef3-11e6-95e4-6567ff374ee0.png) | ![image](https://cloud.githubusercontent.com/assets/627997/19846322/79e76980-9efb-11e6-932e-7730e75dc5f1.png) |
|---|---|
|  |  |

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
