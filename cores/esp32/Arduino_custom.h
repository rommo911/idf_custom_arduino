#ifndef Arduino_h
#define Arduino_h

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_arduino_version.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp32-hal.h"
#include "esp8266-compat.h"
#include "soc/gpio_reg.h"

#include "stdlib_noniso.h"
#include "binary.h"

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352
long map(long, long, long, long, long);

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x) * (x))

#define sei()
#define cli()
#define interrupts() sei()
#define noInterrupts() cli()

#define clockCyclesPerMicrosecond() ((long int)getCpuFrequencyMhz())
#define clockCyclesToMicroseconds(a) ((a) / clockCyclesPerMicrosecond())
#define microsecondsToClockCycles(a) ((a)*clockCyclesPerMicrosecond())

#define lowByte(w) ((uint8_t)((w)&0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))
typedef uint8_t byte;
// avr-libc defines _NOP() since 1.6.2
#ifndef _NOP
#define _NOP()                   \
    do                           \
    {                            \
        __asm__ volatile("nop"); \
    } while (0)
#endif

#define bit(b) (1UL << (b))
#define _BV(b) (1UL << (b))

#define digitalPinToTimer(pin) (0)
#define analogInPinToBit(P) (P)
#if SOC_GPIO_PIN_COUNT <= 32
#define digitalPinToPort(pin) (0)
#define digitalPinToBitMask(pin) (1UL << (pin))
#define portOutputRegister(port) ((volatile uint32_t *)GPIO_OUT_REG)
#define portInputRegister(port) ((volatile uint32_t *)GPIO_IN_REG)
#define portModeRegister(port) ((volatile uint32_t *)GPIO_ENABLE_REG)
#elif SOC_GPIO_PIN_COUNT <= 64
#define digitalPinToPort(pin) (((pin) > 31) ? 1 : 0)
#define digitalPinToBitMask(pin) (1UL << (((pin) > 31) ? ((pin)-32) : (pin)))
#define portOutputRegister(port) ((volatile uint32_t *)((port) ? GPIO_OUT1_REG : GPIO_OUT_REG))
#define portInputRegister(port) ((volatile uint32_t *)((port) ? GPIO_IN1_REG : GPIO_IN_REG))
#define portModeRegister(port) ((volatile uint32_t *)((port) ? GPIO_ENABLE1_REG : GPIO_ENABLE_REG))
#else
#error SOC_GPIO_PIN_COUNT > 64 not implemented
#endif

#define NOT_A_PIN -1
#define NOT_A_PORT -1
#define NOT_AN_INTERRUPT -1
#define NOT_ON_TIMER 0

#define LSBFIRST 0
#define MSBFIRST 1
typedef bool boolean;
typedef unsigned int word;

#ifdef __cplusplus
unsigned long pulseInLong(uint8_t pin, uint8_t state, unsigned long timeout = 1000000L);
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout = 1000000L);

#include <algorithm>
#include <cmath>
#include "WCharacter.h"
#include "WString.h"
#include "stringreplace.hpp"
#include "Stream.h"
#include "Printable.h"
#include "Print.h"
#include "IPAddress.h"
#include "Client.h"
#include "Server.h"
#include "Udp.h"
#include "HardwareSerial.h"
#include "Esp.h"
#include "stringreplace.hpp"

using ::round;
using std::abs;
using std::isinf;
using std::isnan;
using std::max;
using std::min;
#endif
#endif