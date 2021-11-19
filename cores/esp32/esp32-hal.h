/*
 Arduino.h - Main include file for the Arduino SDK
 Copyright (c) 2005-2013 Arduino Team.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef HAL_ESP32_HAL_H_
#define HAL_ESP32_HAL_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "pins_arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp32-hal-cpu.h"
#include "esp32-hal-log.h"
#include "esp32-hal-timer.h"
#include "esp32-hal-bt.h"
#include "esp32-hal-gpio.h"
#include "esp32-hal-i2c-slave.h"
#include "esp32-hal-i2c.h"
#include "esp32-hal-ledc.h"
#include "esp32-hal-matrix.h"
#include "esp32-hal-rmt.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ARDUINO_ISR_ATTR IRAM_ATTR
#define ARDUINO_ISR_FLAG ESP_INTR_FLAG_IRAM
#define ARDUINO_RUNNING_CORE 0
#define ARDUINO_EVENT_RUNNING_CORE 0
    // forward declaration from freertos/portmacro.h
    void vPortYield(void);
    void yield(void);
#define optimistic_yield(u)
#define ESP_REG(addr) *((volatile uint32_t *)(addr))
#define NOP() asm volatile("nop")

#include "esp32-hal-log.h"
    // returns chip temperature in Celsius
    float temperatureRead();
    // enable/disable WDT for the IDLE task on Core 0 (SYSTEM)
    void enableCore0WDT();
    void disableCore0WDT();
    // enable/disable WDT for the IDLE task on Core 1 (Arduino)
    void enableCore1WDT();
    void disableCore1WDT();
    unsigned long micros();
    void delay(uint32_t);
    void delayMicroseconds(uint32_t us);
    unsigned long millis();
    const char *pathToFileName(const char *path);
    void initArduino();

#ifdef __cplusplus
}
#endif


#endif /* HAL_ESP32_HAL_H_ */
