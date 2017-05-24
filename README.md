# BIODIAG - NODE WEBCAM

## Requirements

Arduino IDE - download the latest from arduino

- https://www.arduino.cc/en/Main/Software

Packages for ESP8266 and ArduCAM development on Arduino IDE

- http://www.arducam.com/downloads/ESP8266_UNO/package_ArduCAM_index.json

following libraries are required :

- FS
- WifiManager
- ArduCAM
- ArduinoJson

## Installation

```
git clone git@framagit.org:getlarge/node_webcam.git
```

Edit Arduino/libraries/ArduCAM/memorysaver.h to :

```
#define OV2640_MINI_2MP
```

Then in `config.h.sample` file you may edit the following :

- Name your device for your wifi router ( DHCP )
```
#define MY_ESP8266_HOSTNAME "your_device_name"
```

- Protect the acces point
```
char ap_pass[30]="yourpassword",
```

## Usage

- Open any .ino file of the folder with Arduino IDE
- Edit config.h.sample and copy without .sample extension 
- Uncomment FS.Format the first time you upload
- Comment out FS.format
- Edit login and password in custom_capture.ino.sample and copy without .sample
- Upload the code on the ArduCAM ESP8266 UNO board

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
