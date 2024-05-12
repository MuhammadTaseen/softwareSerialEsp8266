#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_system.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "softuartDrv.hpp"

static const char *TAG = "MAIN";

#define TX_PIN 0
#define RX_PIN 3

#ifdef __cplusplus
extern "C"
{
#endif

    void app_main(void)
    {
        softuart_open(0, 4800.0, RX_PIN, TX_PIN);
        while (1)
        {
            vTaskDelay(10 / portTICK_RATE_MS);
            if (!softuart_available(0))
            {
                continue;
            }

            char c = softuart_read(0);
            if ((uint8_t)c != 0xFF)
                printf("input: %c, 0x%x\n", c, c);
        }
    }

#ifdef __cplusplus
}
#endif
