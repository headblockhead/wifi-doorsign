# wifi-doorsign

A WiFi-connected ePaper display used as a fancy door sign.

https://github.com/user-attachments/assets/719105b5-a09b-4d02-a22d-3bb571632b7c

![IP setup screen](ip_splash.JPEG)
![image screen](hackclub.JPEG)

Wifi credentials are stored under `include/wifi.h` which is gitignore'd, see [`include/wifi_example.h`](include/wifi_example.h) for an example.

## Tasks

### Upload

Upload the firmware to the ESP32 board.

```bash
pio run -e esp32dev --target upload
```

### Monitor

Monitor the serial output of the ESP32 board.

```bash
pio device monitor
```
