#pragma once

#include "stdint.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/* Polling Gpio */
uint8_t GpioDrvConfig(uint8_t gpioNumber, uint8_t direction, uint8_t pullUp, uint8_t pullDown);
uint8_t GpioDrvResetPin(uint8_t gpioNumber);
uint8_t GpioDrvGetLevel(uint8_t gpioNumber);
uint8_t GpioDrvSetLevel(uint8_t gpioNumber, uint8_t level);
uint8_t GpioDrvSetDirection(uint8_t gpioNumber, uint8_t direction);
/* Interrupt Gpio */
uint8_t GpioDrvInterruptConfig(uint8_t gpioNumber, uint8_t intType, uint8_t pullUp, uint8_t pullDown, gpio_isr_t isr_handler, bool addRemove);
uint8_t GpioGetIntrlevel(void);