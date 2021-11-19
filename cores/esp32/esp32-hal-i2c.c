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

#include "esp32-hal-i2c.h"
#include "esp32-hal.h"
#include "esp_attr.h"
#include "esp_system.h"
#include "soc/soc_caps.h"
#include "soc/i2c_periph.h"
#include "hal/i2c_hal.h"
#include "driver/i2c.h"

typedef struct
{
    i2c_hw_cmd_t hw_cmd;
    uint8_t *data;    /*!< data address */
    uint8_t byte_cmd; /*!< to save cmd for one byte command mode */
} i2c_cmd_t;

typedef struct i2c_cmd_link
{
    i2c_cmd_t cmd;             /*!< command in current cmd link */
    struct i2c_cmd_link *next; /*!< next cmd link */
} i2c_cmd_link_t;

typedef struct
{
    i2c_cmd_link_t *head; /*!< head of the command link */
    i2c_cmd_link_t *cur;  /*!< last node of the command link */
    i2c_cmd_link_t *free; /*!< the first node to free of the command link */
    void *free_buffer;    /*!< pointer to the next free data in user's buffer */
    uint32_t free_size;   /*!< remaining size of the user's buffer */
} i2c_cmd_desc_t;

#define I2C_TRANS_BUF_MINIMUM_SIZE (sizeof(i2c_cmd_desc_t) + \
                                    sizeof(i2c_cmd_link_t) * 8)
#define I2C_LINK_RECOMMENDED_SIZE(TRANSACTIONS) (2 * sizeof(i2c_cmd_desc_t) + sizeof(i2c_cmd_desc_t) * (5 * TRANSACTIONS))

i2c_cmd_handle_t i2c_cmd_link_create_static(uint8_t *buffer, uint32_t size)
{
    if (buffer == NULL || size <= sizeof(i2c_cmd_desc_t))
    {
        return NULL;
    }

    i2c_cmd_desc_t *cmd_desc = (i2c_cmd_desc_t *)buffer;
    cmd_desc->head = NULL;
    cmd_desc->cur = NULL;
    cmd_desc->free = NULL;
    cmd_desc->free_buffer = cmd_desc + 1;
    cmd_desc->free_size = size - sizeof(i2c_cmd_desc_t);

    return (i2c_cmd_handle_t)cmd_desc;
}
static inline bool i2c_cmd_link_is_static(i2c_cmd_desc_t *cmd_desc)
{
    return (cmd_desc->free_buffer != NULL);
}
void i2c_cmd_link_delete_static(i2c_cmd_handle_t cmd_handle)
{
    i2c_cmd_desc_t *cmd = (i2c_cmd_desc_t *)cmd_handle;
    if (cmd == NULL || !i2c_cmd_link_is_static(cmd))
    {
        return;
    }
    /* Currently, this function does nothing, but it is not impossible
     * that it will change in a near future. */
}

esp_err_t i2c_master_read_from_device(i2c_port_t i2c_num, uint8_t device_address,
                                      uint8_t *read_buffer, size_t read_size,
                                      TickType_t ticks_to_wait)
{
    esp_err_t err = ESP_OK;
    uint8_t buffer[I2C_TRANS_BUF_MINIMUM_SIZE] = {0};

    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    assert(handle != NULL);

    err = i2c_master_start(handle);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_write_byte(handle, device_address << 1 | I2C_MASTER_READ, true);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_read(handle, read_buffer, read_size, I2C_MASTER_LAST_NACK);
    if (err != ESP_OK)
    {
        goto end;
    }

    i2c_master_stop(handle);
    err = i2c_master_cmd_begin(i2c_num, handle, ticks_to_wait);

end:
    i2c_cmd_link_delete_static(handle);
    return err;
}

////////////////////////////////////////

const char TAG[] = {"i2c"};
typedef volatile struct
{
    bool initialized;
    uint32_t frequency;
#if !CONFIG_DISABLE_HAL_LOCKS
    SemaphoreHandle_t lock;
#endif
} i2c_bus_t;

static i2c_bus_t bus[SOC_I2C_NUM];

bool i2cIsInit(uint8_t i2c_num)
{
    if (i2c_num >= SOC_I2C_NUM)
    {
        return false;
    }
    return bus[i2c_num].initialized;
}

esp_err_t i2cInit(uint8_t i2c_num, int8_t sda, int8_t scl, uint32_t frequency)
{
    if (i2c_num >= SOC_I2C_NUM)
    {
        return ESP_ERR_INVALID_ARG;
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    if (bus[i2c_num].lock == NULL)
    {
        bus[i2c_num].lock = xSemaphoreCreateMutex();
        if (bus[i2c_num].lock == NULL)
        {
            ESP_LOGE(TAG, "xSemaphoreCreateMutex failed");
            return ESP_ERR_NO_MEM;
        }
    }
    // acquire lock
    if (xSemaphoreTake(bus[i2c_num].lock, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "could not acquire lock");
        return ESP_FAIL;
    }
#endif
    if (bus[i2c_num].initialized)
    {
        ESP_LOGE(TAG, "bus is already initialized");
        return ESP_FAIL;
    }

    if (!frequency)
    {
        frequency = 100000UL;
    }
    else if (frequency > 1000000UL)
    {
        frequency = 1000000UL;
    }

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.scl_io_num = (gpio_num_t)scl;
    conf.sda_io_num = (gpio_num_t)sda;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = frequency;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL; // Any one clock source that is available for the specified frequency may be choosen

    esp_err_t ret = i2c_param_config((i2c_port_t)i2c_num, &conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_param_config failed");
    }
    else
    {
        ret = i2c_driver_install((i2c_port_t)i2c_num, conf.mode, 0, 0, 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "i2c_driver_install failed");
        }
        else
        {
            bus[i2c_num].initialized = true;
            bus[i2c_num].frequency = frequency;
        }
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // release lock
    xSemaphoreGive(bus[i2c_num].lock);
#endif
    return ret;
}

esp_err_t i2cDeinit(uint8_t i2c_num)
{
    esp_err_t err = ESP_FAIL;
    if (i2c_num >= SOC_I2C_NUM)
    {
        return ESP_ERR_INVALID_ARG;
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // acquire lock
    if (bus[i2c_num].lock == NULL || xSemaphoreTake(bus[i2c_num].lock, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "could not acquire lock");
        return err;
    }
#endif
    if (!bus[i2c_num].initialized)
    {
        ESP_LOGE(TAG, "bus is not initialized");
    }
    else
    {
        err = i2c_driver_delete((i2c_port_t)i2c_num);
        if (err == ESP_OK)
        {
            bus[i2c_num].initialized = false;
        }
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // release lock
    xSemaphoreGive(bus[i2c_num].lock);
#endif
    return err;
}

esp_err_t i2cWrite(uint8_t i2c_num, uint16_t address, const uint8_t *buff, size_t size, uint32_t timeOutMillis)
{
    esp_err_t ret = ESP_FAIL;
    i2c_cmd_handle_t cmd = NULL;
    if (i2c_num >= SOC_I2C_NUM)
    {
        return ESP_ERR_INVALID_ARG;
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // acquire lock
    if (bus[i2c_num].lock == NULL || xSemaphoreTake(bus[i2c_num].lock, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "could not acquire lock");
        return ret;
    }
#endif
    if (!bus[i2c_num].initialized)
    {
        ESP_LOGE(TAG, "bus is not initialized");
        goto end;
    }

    // short implementation does not support zero size writes (example when scanning) PR in IDF?
    // ret =  i2c_master_write_to_device((i2c_port_t)i2c_num, address, buff, size, timeOutMillis / portTICK_RATE_MS);

    ret = ESP_OK;
    uint8_t cmd_buff[I2C_LINK_RECOMMENDED_SIZE(1)] = {0};
    cmd = i2c_cmd_link_create_static(cmd_buff, I2C_LINK_RECOMMENDED_SIZE(1));
    ret = i2c_master_start(cmd);
    if (ret != ESP_OK)
    {
        goto end;
    }
    ret = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    if (ret != ESP_OK)
    {
        goto end;
    }
    if (size)
    {
        ret = i2c_master_write(cmd, buff, size, true);
        if (ret != ESP_OK)
        {
            goto end;
        }
    }
    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK)
    {
        goto end;
    }
    ret = i2c_master_cmd_begin((i2c_port_t)i2c_num, cmd, timeOutMillis / portTICK_RATE_MS);

end:
    if (cmd != NULL)
    {
        i2c_cmd_link_delete_static(cmd);
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // release lock
    xSemaphoreGive(bus[i2c_num].lock);
#endif
    return ret;
}

esp_err_t i2cRead(uint8_t i2c_num, uint16_t address, uint8_t *buff, size_t size, uint32_t timeOutMillis, size_t *readCount)
{
    esp_err_t ret = ESP_FAIL;
    if (i2c_num >= SOC_I2C_NUM)
    {
        return ESP_ERR_INVALID_ARG;
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // acquire lock
    if (bus[i2c_num].lock == NULL || xSemaphoreTake(bus[i2c_num].lock, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "could not acquire lock");
        return ret;
    }
#endif
    if (!bus[i2c_num].initialized)
    {
        ESP_LOGE(TAG, "bus is not initialized");
    }
    else
    {
        ret = i2c_master_read_from_device((i2c_port_t)i2c_num, address, buff, size, timeOutMillis / portTICK_RATE_MS);
        if (ret == ESP_OK)
        {
            *readCount = size;
        }
        else
        {
            *readCount = 0;
        }
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // release lock
    xSemaphoreGive(bus[i2c_num].lock);
#endif
    return ret;
}

esp_err_t i2c_master_write_read_device(i2c_port_t i2c_num, uint8_t device_address,
                                       const uint8_t *write_buffer, size_t write_size,
                                       uint8_t *read_buffer, size_t read_size,
                                       TickType_t ticks_to_wait)
{
    esp_err_t err = ESP_OK;
    uint8_t buffer[I2C_TRANS_BUF_MINIMUM_SIZE] = {0};

    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    assert(handle != NULL);

    err = i2c_master_start(handle);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_write_byte(handle, device_address << 1 | I2C_MASTER_WRITE, true);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_write(handle, write_buffer, write_size, true);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_start(handle);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_write_byte(handle, device_address << 1 | I2C_MASTER_READ, true);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_read(handle, read_buffer, read_size, I2C_MASTER_LAST_NACK);
    if (err != ESP_OK)
    {
        goto end;
    }
    i2c_master_stop(handle);
    err = i2c_master_cmd_begin(i2c_num, handle, ticks_to_wait);

end:
    i2c_cmd_link_delete_static(handle);
    return err;
}

esp_err_t i2cWriteReadNonStop(uint8_t i2c_num, uint16_t address, const uint8_t *wbuff, size_t wsize, uint8_t *rbuff, size_t rsize, uint32_t timeOutMillis, size_t *readCount)
{
    esp_err_t ret = ESP_FAIL;
    if (i2c_num >= SOC_I2C_NUM)
    {
        return ESP_ERR_INVALID_ARG;
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // acquire lock
    if (bus[i2c_num].lock == NULL || xSemaphoreTake(bus[i2c_num].lock, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "could not acquire lock");
        return ret;
    }
#endif
    if (!bus[i2c_num].initialized)
    {
        ESP_LOGE(TAG, "bus is not initialized");
    }
    else
    {
        ret = i2c_master_write_read_device((i2c_port_t)i2c_num, address, wbuff, wsize, rbuff, rsize, timeOutMillis / portTICK_RATE_MS);
        if (ret == ESP_OK)
        {
            *readCount = rsize;
        }
        else
        {
            *readCount = 0;
        }
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // release lock
    xSemaphoreGive(bus[i2c_num].lock);
#endif
    return ret;
}

esp_err_t i2cSetClock(uint8_t i2c_num, uint32_t frequency)
{
    esp_err_t ret = ESP_FAIL;
    if (i2c_num >= SOC_I2C_NUM)
    {
        return ESP_ERR_INVALID_ARG;
    }
#if !CONFIG_DISABLE_HAL_LOCKS
    // acquire lock
    if (bus[i2c_num].lock == NULL || xSemaphoreTake(bus[i2c_num].lock, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "could not acquire lock");
        return ret;
    }
#endif
    if (!bus[i2c_num].initialized)
    {
        ESP_LOGE(TAG, "bus is not initialized");
        goto end;
    }
    if (bus[i2c_num].frequency == frequency)
    {
        ret = ESP_OK;
        goto end;
    }
    if (!frequency)
    {
        frequency = 100000UL;
    }
    else if (frequency > 1000000UL)
    {
        frequency = 1000000UL;
    }
// Freq limitation when using different clock sources
#define I2C_CLK_LIMIT_REF_TICK (1 * 1000 * 1000 / 20) /*!< Limited by REF_TICK, no more than REF_TICK/20*/
#define I2C_CLK_LIMIT_APB (80 * 1000 * 1000 / 20)     /*!< Limited by APB, no more than APB/20*/
#define I2C_CLK_LIMIT_RTC (20 * 1000 * 1000 / 20)     /*!< Limited by RTC, no more than RTC/20*/
#define I2C_CLK_LIMIT_XTAL (40 * 1000 * 1000 / 20)    /*!< Limited by RTC, no more than XTAL/20*/

    typedef struct
    {
        uint8_t character; /*!< I2C source clock characteristic */
        uint32_t clk_freq; /*!< I2C source clock frequency */
    } i2c_clk_alloc_t;

    // i2c clock characteristic, The order is the same as i2c_sclk_t.
    static i2c_clk_alloc_t i2c_clk_alloc[I2C_SCLK_MAX] = {
        {0, 0},
#if SOC_I2C_SUPPORT_APB
        {0, I2C_CLK_LIMIT_APB}, /*!< I2C APB clock characteristic*/
#endif
#if SOC_I2C_SUPPORT_XTAL
        {0, I2C_CLK_LIMIT_XTAL}, /*!< I2C XTAL characteristic*/
#endif
#if SOC_I2C_SUPPORT_RTC
        {I2C_SCLK_SRC_FLAG_LIGHT_SLEEP | I2C_SCLK_SRC_FLAG_AWARE_DFS, I2C_CLK_LIMIT_RTC}, /*!< I2C 20M RTC characteristic*/
#endif
#if SOC_I2C_SUPPORT_REF_TICK
        {I2C_SCLK_SRC_FLAG_AWARE_DFS, I2C_CLK_LIMIT_REF_TICK}, /*!< I2C REF_TICK characteristic*/
#endif
    };

    i2c_sclk_t src_clk = I2C_SCLK_DEFAULT;
    ret = ESP_OK;
    for (i2c_sclk_t clk = I2C_SCLK_DEFAULT + 1; clk < I2C_SCLK_MAX; clk++)
    {
#if CONFIG_IDF_TARGET_ESP32S3
        if (clk == I2C_SCLK_RTC)
        { // RTC clock for s3 is unaccessable now.
            continue;
        }
#endif
        if (frequency <= i2c_clk_alloc[clk].clk_freq)
        {
            src_clk = clk;
            break;
        }
    }
    if (src_clk == I2C_SCLK_MAX)
    {
        ESP_LOGE(TAG, "clock source could not be selected");
        ret = ESP_FAIL;
    }
    else
    {
        i2c_hal_context_t hal;
        hal.dev = I2C_LL_GET_HW(i2c_num);
        i2c_hal_set_bus_timing(&(hal), frequency, src_clk);
        bus[i2c_num].frequency = frequency;
    }

end:
#if !CONFIG_DISABLE_HAL_LOCKS
    // release lock
    xSemaphoreGive(bus[i2c_num].lock);
#endif
    return ret;
}

esp_err_t i2cGetClock(uint8_t i2c_num, uint32_t *frequency)
{
    if (i2c_num >= SOC_I2C_NUM)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!bus[i2c_num].initialized)
    {
        ESP_LOGE(TAG, "bus is not initialized");
        return ESP_FAIL;
    }
    *frequency = bus[i2c_num].frequency;
    return ESP_OK;
}
