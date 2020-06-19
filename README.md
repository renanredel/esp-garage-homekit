# ESP-Garage-Homekit

This is simple project to controll the garage door with Homekit with ESP32 module. Based on the Garage example from [Esp-Homekit-Demo][plhh]. It also uses the hall sensor inside of ESP32 as a determinant of the state of the garage door (open or closed).

## Prerequisite

Name           | Link
-------------- | --------------------------------------------------
ESP-IDF v3.2.3 | [https://github.com/espressif/esp-idf][pldb]
ESP-Homekit    | [https://github.com/maximkulkin/esp-homekit][plgh]

## Download

After installed ESP-IDF and Homekit:

```sh
$ git clone https://github.com/renanredel/esp-garage-homekit.git
```

## Configuration

1. Enter in `wifi.h` and edit:

  ```sh
  #define WIFI_SSID "SSID"
  #define WIFI_PASSWORD "password"
  ```

2. Edit the pin to control the door in `main.c`. You can set an relay, control, etc.

  ```
  const int control_gpio = 25;
  ```

## Build

```sh
$ cd esp-garage-homekit
$ make menuconfig
$ make flash monitor
```

Make sure change the USB port in Menuconfig.

## Setup Code

You can change in `garage.c`

Default setup code:

### 111-11-111

## Video demo

[![ESP32 Garage Door Demo](https://yt-embed.herokuapp.com/embed?v=RDww87Yjhqk)](https://www.youtube.com/watch?v=RDww87Yjhqk)

[pldb]: https://github.com/espressif/esp-idf
[plgh]: https://github.com/maximkulkin/esp-homekit
[plhh]: https://github.com/maximkulkin/esp-homekit-demo
