# NODE WEBCAM

## Requirements

Arduino IDE - download the latest from arduino

- https://www.arduino.cc/en/Main/Software

Packages for ESP8266 and ArduCAM development on Arduino IDE

- http://www.arducam.com/downloads/ESP8266_UNO/package_ArduCAM_index.json

following libraries are required :

- ArduCAM
- ArduinoJson
- FS
- Pubsubclient
- WifiManager

## Installation

```
git clone https://github/getlarge/NodeWebcam.git
```

Edit Arduino/libraries/ArduCAM/memorysaver.h to :

```
#define OV2640_MINI_2MP
```

Then in `config.h` file you may edit the following :

- Protect the acces point
```
char ap_pass[30]="yourpassword",
```

## Usage

- Open any .ino file of the folder with Arduino IDE
- Edit your preferences
- Set resetConfig to true, to make FS format and wifiManager resetting
- Upload the code on your ESP8266 board

## Reference

- resolutions:
- 0 = 160x120
- 1 = 176x144
- 2 = 320x240
- 3 = 352x288
- 4 = 640x480
- 5 = 800x600
- 6 = 1024x768
- 7 = 1280x1024
- 8 = 1600x1200


## Dev

Go to the dev branch for the latest and unstable development
