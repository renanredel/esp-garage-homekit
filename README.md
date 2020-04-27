# ESP-Garage-Homekit

This is simple project to controll the garage door with Homekit with ESP32 module.
Based on the Garage example from [Esp-Homekit-Demo][PlHh].
It also uses the hall sensor inside of ESP32 as a determinant of the state of the garage door (open or closed).
Still in development.

### Prerequisite

| Name | Link |
| ------ | ------ |
| ESP-IDF v3.2.3 | [https://github.com/espressif/esp-idf][PlDb] |
| ESP-Homekit | [https://github.com/maximkulkin/esp-homekit][PlGh] |

### Download

After installed ESP-IDF and Homekit:
```sh
$ git clone https://github.com/renanredel/esp-garage-homekit.git
```

### Configuration

1. Enter in ``wifi.h`` and edit:
```sh
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "password"
```

### Build 

```sh
$ cd esp-garage-homekit
$ make menuconfig
$ make flash monitor
```
Make sure change the USB port in Menuconfig.

### Setup Code

You can change in ```garage.c``` 

Default setup code:
## 111-11-111

   [PlDb]: <https://github.com/espressif/esp-idf>
   [PlGh]: <https://github.com/maximkulkin/esp-homekit>
   [PlHh]: <https://github.com/maximkulkin/esp-homekit-demo>
