// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef __ARDUHAL_LOG_H__
#define __ARDUHAL_LOG_H__
#pragma once
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#define USE_ESP_IDF_LOG
#ifdef __cplusplus

extern "C"
{
#endif

#include "sdkconfig.h"
#include "esp_timer.h"
#include "esp32/rom/ets_sys.h"
#include "esp_log.h"

#define ARDUHAL_LOG_LEVEL_NONE       (0)
#define ARDUHAL_LOG_LEVEL_ERROR      (1)
#define ARDUHAL_LOG_LEVEL_WARN       (2)
#define ARDUHAL_LOG_LEVEL_INFO       (3)
#define ARDUHAL_LOG_LEVEL_DEBUG      (4)
#define ARDUHAL_LOG_LEVEL_VERBOSE    (5)

#ifndef CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL
#define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL ARDUHAL_LOG_LEVEL_NONE
#endif


#define ARDUHAL_LOG_LEVEL CORE_DEBUG_LEVEL

#ifndef CONFIG_ARDUHAL_LOG_COLORS
#define CONFIG_ARDUHAL_LOG_COLORS 0
#endif

#define _TAG "TEST"


#define ARDUHAL_SHORT_LOG_FORMAT(letter, format)  ARDUHAL_LOG_COLOR_ ## letter format ARDUHAL_LOG_RESET_COLOR "\r\n"
#define ARDUHAL_LOG_FORMAT(letter, format)  ARDUHAL_LOG_COLOR_ ## letter "[%6u][" #letter "][%s:%u] %s(): " format ARDUHAL_LOG_RESET_COLOR "\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), pathToFileName(__FILE__), __LINE__, __FUNCTION__

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_VERBOSE
#ifndef USE_ESP_IDF_LOG
#else
#define log_v(format, ...) do {ESP_LOG_LEVEL_LOCAL(ESP_LOG_VERBOSE, _TAG, format, ##__VA_ARGS__);}while(0)
#define isr_log_v(format, ...) do {ets_printf(LOG_FORMAT(V, format), esp_log_timestamp(), _TAG, ##__VA_ARGS__);}while(0)
#define log_buf_v(b,l) do {ESP_LOG_BUFFER_HEXDUMP(_TAG, b, l, ESP_LOG_VERBOSE);}while(0)
#endif
#else
#define log_v(format, ...)
#define isr_log_v(format, ...)
#define log_buf_v(b,l)
#endif

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
#ifndef USE_ESP_IDF_LOG
#else
#define log_d(format, ...) do {ESP_LOG_LEVEL_LOCAL(ESP_LOG_DEBUG, _TAG, format, ##__VA_ARGS__);}while(0)
#define isr_log_d(format, ...) do {ets_printf(LOG_FORMAT(D, format), esp_log_timestamp(), _TAG, ##__VA_ARGS__);}while(0)
#define log_buf_d(b,l) do {ESP_LOG_BUFFER_HEXDUMP(_TAG, b, l, ESP_LOG_DEBUG);}while(0)
#endif
#else
#define log_d(format, ...)
#define isr_log_d(format, ...)
#define log_buf_d(b,l)
#endif

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
#ifndef USE_ESP_IDF_LOG
#else
#define log_i(format, ...) do {ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO, _TAG, format, ##__VA_ARGS__);}while(0)
#define isr_log_i(format, ...) do {ets_printf(LOG_FORMAT(I, format), esp_log_timestamp(), _TAG, ##__VA_ARGS__);}while(0)
#define log_buf_i(b,l) do {ESP_LOG_BUFFER_HEXDUMP(_TAG, b, l, ESP_LOG_INFO);}while(0)
#endif
#else
#define log_i(format, ...)
#define isr_log_i(format, ...)
#define log_buf_i(b,l)
#endif

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_WARN
#ifndef USE_ESP_IDF_LOG
#else
#define log_w(format, ...) do {ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARN, _TAG, format, ##__VA_ARGS__);}while(0)
#define isr_log_w(format, ...) do {ets_printf(LOG_FORMAT(W, format), esp_log_timestamp(), _TAG, ##__VA_ARGS__);}while(0)
#define log_buf_w(b,l) do {ESP_LOG_BUFFER_HEXDUMP(_TAG, b, l, ESP_LOG_WARN);}while(0)
#endif
#else
#define log_w(format, ...)
#define isr_log_w(format, ...)
#define log_buf_w(b,l)
#endif

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_ERROR
#ifndef USE_ESP_IDF_LOG
#else
#define log_e(format, ...) do {ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR,_TAG, format, ##__VA_ARGS__);}while(0)
#define isr_log_e(format, ...) do {ets_printf(LOG_FORMAT(E, format), esp_log_timestamp(), _TAG, ##__VA_ARGS__);}while(0)
#define log_buf_e(b,l) do {ESP_LOG_BUFFER_HEXDUMP(_TAG, b, l, ESP_LOG_ERROR);}while(0)
#endif
#else
#define log_e(format, ...)
#define isr_log_e(format, ...)
#define log_buf_e(b,l)
#endif

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_NONE
#ifndef USE_ESP_IDF_LOG
#else
#define log_n(format, ...) do {ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR, _TAG, format, ##__VA_ARGS__);}while(0)
#define isr_log_n(format, ...) do {ets_printf(LOG_FORMAT(E, format), esp_log_timestamp(), _TAG, ##__VA_ARGS__);}while(0)
#define log_buf_n(b,l) do {ESP_LOG_BUFFER_HEXDUMP(_TAG, b, l, ESP_LOG_ERROR);}while(0)
#endif
#else
#define log_n(format, ...)
#define isr_log_n(format, ...)
#define log_buf_n(b,l)
#endif



#ifdef __cplusplus
}
#endif

#endif /* __ESP_LOGGING_H__ */
