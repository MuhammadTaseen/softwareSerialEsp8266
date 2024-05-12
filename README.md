# Firmware for Software Serial with ESP8266 RTOS SDK



## Introduction

The software is helping to stand alone run the software serial on `RX line` of esp01(having only one uart) using esp8266 with its esp8266 rtos sdk.

The main part is to develop the main Soft Uart module using the available  [esp open freeRtos](https://github.com/SuperHouse/esp-open-rtos/tree/master/extras/softuart). The main code was in `c`. But this available module is converted and ready to build with the old [ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK) of `release/v2.1`

## Usage

1. The main directory contain 2x directories

- The main directory, having the main user app
- The sofrwarDrv directory having the main driver which is ready to build with the `ESP8266 RTOS SDK`


## Inspired from 

The main inspiration to convert this library for esp8266 rtos sdk is to run the only `RX` line of `UART_0(used for loggs)` with rto's rather then using arduino software serial lib.


## How to build

1. Clone [ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK) `release/v2.1`.
2. Now export the RTO's sdk environment and open the directly to build the LED strip
3. The step for exporting the RTO's SDK will be explain in next version.
4. After opening the directory just build the code


```sh

idf.py build

```

## References

- **ESP_OPEN_RTOS:** `https://github.com/SuperHouse/esp-open-rtos/tree/master/extras/softuart`
- **ESP8266RTO's Sdk:** `https://github.com/espressif/ESP8266_RTOS_SDK`

- **Language:** `C/C++`
- **SDK:** `ESP8266 RTOS SDK`

## Author

**Developer Name:** `M.Taseen`


