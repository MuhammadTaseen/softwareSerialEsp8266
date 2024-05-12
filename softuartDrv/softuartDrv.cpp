/*
 * Softuart
 *
 * Copyright (C) 2017 Ruslan V. Uss <unclerus@gmail.com>
 * Copyright (C) 2016 Bernhard Guillon <Bernhard.Guillon@web.de>
 *
 * This code is based on Softuart from here [1] and reworked to
 * fit into esp-open-rtos.
 *
 * it fits my needs to read the GY-GPS6MV2 module with 9600 8n1
 *
 * Original Copyright:
 * Copyright (c) 2015 plieningerweb
 *
 * MIT Licensed as described in the file LICENSE
 *
 * 1 https://github.com/plieningerweb/esp8266-software-uart
 */
#include <stdint.h>
#include <stdio.h>

#include "esp_system.h"
#include <rom/ets_sys.h>
#include "esp_timer.h"

#include "driver/gpio.h"
#include "esp8266/gpio_register.h"

#include "GpioDrv.hpp"
#include "softuartDrv.hpp"

volatile uint16_t _rx_delay_centering;
volatile uint16_t _rx_delay_intrabit;
volatile uint16_t _rx_delay_stopbit;

#define SOFTUART_DEBUG
#ifdef SOFTUART_DEBUG
#define debug(fmt, ...) printf("%s: " fmt "\n", "SOFTUART", ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

typedef struct
{
    char receive_buffer[SOFTUART_MAX_RX_BUFF];
    uint8_t receive_buffer_tail;
    uint8_t receive_buffer_head;
    uint8_t buffer_overflow;
} softuart_buffer_t;

typedef struct
{
    uint8_t rx_pin, tx_pin;
    uint32_t baudrate;
    volatile softuart_buffer_t buffer;
    uint16_t bit_time;
} softuart_t;

static softuart_t uarts[SOFTUART_MAX_UARTS] = {{0}};

inline static int8_t find_uart_by_rx(uint8_t rx_pin)
{
    for (uint8_t i = 0; i < SOFTUART_MAX_UARTS; i++)
        if (uarts[i].baudrate && uarts[i].rx_pin == rx_pin)
            return i;

    return -1;
}

uint16_t subtract_cap(uint16_t num, uint16_t sub)
{
    if (num > sub)
        return num - sub;
    else
        return 1;
}

// GPIO interrupt handler

static void IRAM_ATTR handle_rx(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    uint32_t gpioLevel = gpio_get_level((gpio_num_t)gpio_num);

    // find uart
    int8_t uart_no = find_uart_by_rx(gpio_num);
    if (uart_no < 0)
        return;

    softuart_t *uart = uarts + uart_no;

    // Disable interrupt
    gpio_isr_handler_remove((gpio_num_t)gpio_num);

    // Wait till start bit is half over so we can sample the next one in the center
    ets_delay_us(_rx_delay_centering);

    // Now sample bits
    uint8_t d = 0;
    uint32_t start_time = 0x7FFFFFFF & (esp_timer_get_time() / 1);

    for (uint8_t i = 0; i < 8; i++)
    {
        ets_delay_us(_rx_delay_intrabit);

        // Shift d to the right
        d >>= 1;

        // Read bit
        if (gpio_get_level((gpio_num_t)uart->rx_pin))
        {
            // If high, set msb of 8bit to 1
            d |= 0x80;
        }
    }

    // Store byte in buffer
    // If buffer full, set the overflow flag and return
    uint8_t next = (uart->buffer.receive_buffer_tail + 1) % SOFTUART_MAX_RX_BUFF;
    if (next != uart->buffer.receive_buffer_head)
    {
        // save new data in buffer: tail points to where byte goes
        uart->buffer.receive_buffer[uart->buffer.receive_buffer_tail] = d; // save new byte
        uart->buffer.receive_buffer_tail = next;
    }
    else
    {
        uart->buffer.buffer_overflow = 1;
    }

    // Wait for stop bit
    ets_delay_us(_rx_delay_stopbit);

    // Done, reenable interrupt
    gpio_isr_handler_remove((gpio_num_t)uart->rx_pin);
    GpioDrvInterruptConfig(uart->rx_pin, GPIO_INTR_NEGEDGE, 0, 0, handle_rx, true);
}

static bool check_uart_no(uint8_t uart_no)
{
    if (uart_no >= SOFTUART_MAX_UARTS)
    {
        debug("Invalid uart number %d, %d max", uart_no, SOFTUART_MAX_UARTS);
        return false;
    }

    return true;
}

static bool check_uart_enabled(uint8_t uart_no)
{
    if (!uarts[uart_no].baudrate)
    {
        debug("Uart %d is disabled", uart_no);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Public
///////////////////////////////////////////////////////////////////////////////

bool softuart_open(uint8_t uart_no, float baudrate, uint8_t rx_pin, uint8_t tx_pin)
{
    // do some checks
    printf("Opening the UART \n");
    if (!check_uart_no(uart_no))
        return false;
    if (baudrate == 0)
    {
        debug("Invalid baudrate");
        return false;
    }
    for (uint8_t i = 0; i < SOFTUART_MAX_UARTS; i++)
        if (uarts[i].baudrate && i != uart_no && (uarts[i].rx_pin == rx_pin || uarts[i].tx_pin == tx_pin || uarts[i].rx_pin == tx_pin || uarts[i].tx_pin == rx_pin))
        {
            debug("Cannot share pins between uarts");
            return false;
        }

    softuart_close(uart_no);

    softuart_t *uart = uarts + uart_no;

    uart->baudrate = baudrate;
    uart->rx_pin = rx_pin;
    uart->tx_pin = tx_pin;

    float us = ((1.0 / 4800.0) * 1000000.0);

    uart->bit_time = ((uint16_t)us);

    _rx_delay_centering = ((uint16_t)us / 2) + 80;
    _rx_delay_intrabit = (uint16_t)us;
    _rx_delay_stopbit = (uint16_t)us;
    printf("delay %d \n", (uint16_t)us);

    // Setup Rx
    GpioDrvConfig(rx_pin, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE);

    // Setup Tx
    GpioDrvConfig(tx_pin, GPIO_MODE_OUTPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE);
    GpioDrvSetLevel(tx_pin, 1);

    // Setup the interrupt handler to get the start bit

    GpioDrvInterruptConfig(rx_pin, GPIO_INTR_NEGEDGE, 0, 0, handle_rx, false);

    ets_delay_us(1000); // TODO: not sure if it really needed
    printf("Opening the UART Done\n");

    return true;
}

bool softuart_close(uint8_t uart_no)
{
    if (!check_uart_no(uart_no))
        return false;
    softuart_t *uart = uarts + uart_no;

    if (!uart->baudrate)
        return true;

    // Remove interrupt
    gpio_isr_handler_remove((gpio_num_t)uart->rx_pin);
    // Mark as unused
    uart->baudrate = 0;

    return true;
}

bool softuart_put(uint8_t uart_no, char c)
{
    if (!check_uart_no(uart_no))
        return false;
    if (!check_uart_enabled(uart_no))
        return false;
    softuart_t *uart = uarts + uart_no;

    uint32_t start_time = 0x7FFFFFFF & (esp_timer_get_time() / 1);
    GpioDrvSetLevel(uart->tx_pin, 0);

    for (uint8_t i = 0; i <= 8; i++)
    {
        while ((0x7FFFFFFF & (esp_timer_get_time() / 1)) < (start_time + (uart->bit_time * (i + 1))))
        {
            if ((0x7FFFFFFF & (esp_timer_get_time() / 1)) < start_time)
                break;
        }
        GpioDrvSetLevel(uart->tx_pin, c & (1 << i));
    }

    while ((0x7FFFFFFF & (esp_timer_get_time() / 1)) < (start_time + (uart->bit_time * 9)))
    {
        if ((0x7FFFFFFF & (esp_timer_get_time() / 1)) < start_time)
            break;
    }
    GpioDrvSetLevel(uart->tx_pin, 1);
    ets_delay_us(uart->bit_time * 6);

    return true;
}

bool softuart_puts(uint8_t uart_no, const char *s)
{
    while (*s)
    {
        if (!softuart_put(uart_no, *s++))
            return false;
    }

    return true;
}

bool softuart_available(uint8_t uart_no)
{
    // debug("checking \n");
    if (!check_uart_no(uart_no))
        return false;
    if (!check_uart_enabled(uart_no))
        return false;
    softuart_t *uart = uarts + uart_no;

    return (uart->buffer.receive_buffer_tail + SOFTUART_MAX_RX_BUFF - uart->buffer.receive_buffer_head) % SOFTUART_MAX_RX_BUFF;
}

uint8_t softuart_read(uint8_t uart_no)
{
    if (!check_uart_no(uart_no))
        return 0;
    if (!check_uart_enabled(uart_no))
        return 0;
    softuart_t *uart = uarts + uart_no;

    // Empty buffer?
    if (uart->buffer.receive_buffer_head == uart->buffer.receive_buffer_tail)
        return 0;
    
    // Read from "head"
    uint8_t d = uart->buffer.receive_buffer[uart->buffer.receive_buffer_head]; // grab next byte
    uart->buffer.receive_buffer_head = (uart->buffer.receive_buffer_head + 1) % SOFTUART_MAX_RX_BUFF;
    return d;
}
