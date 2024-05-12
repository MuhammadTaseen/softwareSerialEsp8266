#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "GpioDrv.hpp"

#define GPIO_OUTPUT_IO_0 18
#define GPIO_OUTPUT_IO_1 19
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_0 4
#define GPIO_INPUT_IO_1 5
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_0) | (1ULL << GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

uint32_t volatile gpioLevel = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    gpioLevel = gpio_get_level((gpio_num_t)gpio_num);
}

/**
 * @brief Fucntion used to config. a gpio
 *
 * @param gpioNumber pin number
 * @param direction initial direction of pin either input or output
 *
 *  0. enumerator GPIO_MODE_DISABLE
        GPIO mode : disable input and output
    1. enumerator GPIO_MODE_INPUT
        GPIO mode : input only
    2. enumerator GPIO_MODE_OUTPUT
        GPIO mode : output only mode
    3. enumerator GPIO_MODE_OUTPUT_OD
        GPIO mode : output only with open-drain mode
    4. enumerator GPIO_MODE_INPUT_OUTPUT_OD
        GPIO mode : output and input with open-drain mode
    5. enumerator GPIO_MODE_INPUT_OUTPUT
        GPIO mode : output and input mode
 *
 * @param pullUp
 *
*   0. enumerator GPIO_PULLUP_DISABLE
        Disable GPIO pull-up resistor
    1. enumerator GPIO_PULLUP_ENABLE
        Enable GPIO pull-up resistor
 * @param pullDown
 *  0. enumerator GPIO_PULLDOWN_DISABLE
        Disable GPIO pull-down resistor
    1. enumerator GPIO_PULLDOWN_ENABLE
        Enable GPIO pull-down resistor
 *
 * @return uint8_t
 */
uint8_t GpioDrvConfig(uint8_t gpioNumber, uint8_t direction, uint8_t pullUp, uint8_t pullDown)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = (gpio_mode_t)direction;
    io_conf.pin_bit_mask = ((1ULL << (gpio_num_t)gpioNumber));
    io_conf.pull_down_en = (gpio_pulldown_t)pullDown;
    io_conf.pull_up_en = (gpio_pullup_t)pullUp;
    return (gpio_config(&io_conf) == ESP_OK ? 1 : 0);
    // esp_err_t err;

    // err = gpio_set_direction((gpio_num_t)gpioNumber, (gpio_mode_t)direction);

    // /*  enum gpio_mode_t
    //   Values:
    //   GPIO_MODE_DISABLE
    //   GPIO_MODE_INPUT
    //   GPIO_MODE_OUTPUT
    //   GPIO_MODE_OUTPUT_OD
    //   GPIO_MODE_INPUT_OUTPUT_OD --- GPIO mode : output and input with open-drain
    //  mode GPIO_MODE_INPUT_OUTPUT  */

    // err = gpio_set_pull_mode((gpio_num_t)gpioNumber, (gpio_pull_mode_t)pullUp);

    // /*enum gpio_pull_mode_t
    //   values:
    //   GPIO_PULLUP_ONLY = 0
    //   GPIO_PULLDOWN_ONLY = 1
    //   GPIO_PULLUP_PULLDOWN = 2
    //   GPIO_FLOATING = 3 */
    // // switch (err)
    // // {
    // //     case ESP_OK:
    // //         return DRV_SUCESSFUL;
    // //     case ESP_FAIL:
    // //         return DRV_FAIL;

    // //     default:
    // //         return DRV_FAIL;
    // // }
    // return (err == ESP_OK ? 1 : 0);
}

/**
 * @brief Reset an gpio to default state (select gpio function, enable pullup and disable input and output).
 * This function also configures the IOMUX for this pin to the GPIO function, and disconnects any other peripheral output configured via GPIO Matrix.
 * @param gpioNumber number of GPIO
 * @return uint8_t always return ESP_OK = 0
 */
uint8_t GpioDrvResetPin(uint8_t gpioNumber)
{
    // return gpio_reset_pin(gpioNumber);
    return 0;
}

/**
 * @brief Fcuntion used to get the information from the gpio pin
 *
 * warning! If the pad is not configured for input (or input and output) the returned value is always 0.
 *
 * @param gpioNumber
 * @return uint8_t
 */
uint8_t GpioDrvGetLevel(uint8_t gpioNumber)
{
    return gpio_get_level((gpio_num_t)gpioNumber);
}

uint8_t GpioDrvSetDirection(uint8_t gpioNumber, uint8_t direction)
{
    return gpio_set_direction((gpio_num_t)gpioNumber, (gpio_mode_t)direction);
}
/**
 * @brief
 *
 * @param gpioNumber
 * @param level
 * @return uint8_t
 */
uint8_t GpioDrvSetLevel(uint8_t gpioNumber, uint8_t level)
{
    return (gpio_set_level((gpio_num_t)gpioNumber, level) == ESP_OK ? 1 : 0);
}

uint8_t GpioDrvInterruptConfig(uint8_t gpioNumber, uint8_t intType, uint8_t pullUp, uint8_t pullDown, gpio_isr_t isr_handler, bool addRemove)
{
    if (!addRemove)
    {
        // zero-initialize the config structure.
        gpio_config_t io_conf;
        // interrupt of rising edge
        io_conf.intr_type = (gpio_int_type_t)intType;
        // bit mask of the pins, use GPIO4/5 here
        io_conf.pin_bit_mask = ((1ULL << gpioNumber));
        // set as input mode
        io_conf.mode = GPIO_MODE_INPUT;
        // enable pull-up mode
        io_conf.pull_up_en = (gpio_pullup_t)1;
        gpio_config(&io_conf);

        // install gpio isr service
        gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
        // hook isr handler for specific gpio pin
        gpio_isr_handler_add((gpio_num_t)gpioNumber, isr_handler, (void *)gpioNumber);
    }
    else
    {
        gpio_isr_handler_add((gpio_num_t)gpioNumber, isr_handler, (void *)gpioNumber);
    }
    return 0;
}

uint8_t GpioGetIntrlevel(void)
{
    return gpioLevel;
}