# wifi-doorsign

Wifi credentials are stored under `include/wifi.h` which is gitignore'd, see [`include/wifi_example.h`](include/wifi_example.h) for an example.

## Tasks

### Upload

Upload the firmware to the ESP32 board.

```bash
pio run -e esp32dev --target upload
```
