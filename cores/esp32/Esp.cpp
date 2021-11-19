/*
 Esp.cpp - ESP31B-specific APIs
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.

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

#include "Arduino_custom.h"
#include "Esp.h"
#include "esp_sleep.h"
#include "esp_spi_flash.h"
#include "esp_app_format.h"
#include <memory>
#include <soc/soc.h>
#include <esp_partition.h>
extern "C"
{
#include "esp_ota_ops.h"
#include "esp_image_format.h"
}
#include <MD5Builder.h>

#include "esp_system.h"
#ifdef ESP_IDF_VERSION_MAJOR // IDF 4+
#if CONFIG_IDF_TARGET_ESP32  // ESP32/PICO-D4
#include "esp32/rom/spi_flash.h"
#include "soc/efuse_reg.h"
#define ESP_FLASH_IMAGE_BASE 0x1000 // Flash offset containing flash size and spi mode
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/spi_flash.h"
#include "soc/efuse_reg.h"
#define ESP_FLASH_IMAGE_BASE 0x1000
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/spi_flash.h"
#define ESP_FLASH_IMAGE_BASE 0x0000 // Esp32c3 is located at 0x0000
#else
#error Target CONFIG_IDF_TARGET is not supported
#endif
#else // ESP32 Before IDF 4.0
#include "rom/spi_flash.h"
#define ESP_FLASH_IMAGE_BASE 0x1000
#endif

/**
 * User-defined Literals
 *  usage:
 *
 *   uint32_t = test = 10_MHz; // --> 10000000
 */

unsigned long long operator"" _kHz(unsigned long long x)
{
    return x * 1000;
}

unsigned long long operator"" _MHz(unsigned long long x)
{
    return x * 1000 * 1000;
}

unsigned long long operator"" _GHz(unsigned long long x)
{
    return x * 1000 * 1000 * 1000;
}

unsigned long long operator"" _kBit(unsigned long long x)
{
    return x * 1024;
}

unsigned long long operator"" _MBit(unsigned long long x)
{
    return x * 1024 * 1024;
}

unsigned long long operator"" _GBit(unsigned long long x)
{
    return x * 1024 * 1024 * 1024;
}

unsigned long long operator"" _kB(unsigned long long x)
{
    return x * 1024;
}

unsigned long long operator"" _MB(unsigned long long x)
{
    return x * 1024 * 1024;
}

unsigned long long operator"" _GB(unsigned long long x)
{
    return x * 1024 * 1024 * 1024;
}

EspClass ESP;

void EspClass::deepSleep(uint32_t time_us)
{
    esp_deep_sleep(time_us);
}

void EspClass::restart(void)
{
    esp_restart();
}

uint32_t EspClass::getHeapSize(void)
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    return info.total_free_bytes + info.total_allocated_bytes;
}

uint32_t EspClass::getFreeHeap(void)
{
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t EspClass::getMinFreeHeap(void)
{
    return heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t EspClass::getMaxAllocHeap(void)
{
    return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
}

uint32_t EspClass::getPsramSize(void)
{
	multi_heap_info_t info;
	heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
	return info.total_free_bytes + info.total_allocated_bytes;
}

static uint32_t sketchSize(sketchSize_t response)
{
    esp_image_metadata_t data;
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running)
        return 0;
    const esp_partition_pos_t running_pos = {
        .offset = running->address,
        .size = running->size,
    };
    data.start_addr = running_pos.offset;
    esp_image_verify(ESP_IMAGE_VERIFY, &running_pos, &data);
    if (response)
    {
        return running_pos.size - data.image_len;
    }
    else
    {
        return data.image_len;
    }
}

uint32_t EspClass::getSketchSize()
{
    return sketchSize(SKETCH_SIZE_TOTAL);
}

std::string EspClass::getSketchMD5_string()
{
    const String ret = getSketchMD5();
    return std::string(ret.c_str());
}
String EspClass::getSketchMD5()
{
    static String result;
    if (result.length())
    {
        return result;
    }
    uint32_t lengthLeft = getSketchSize();

    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running)
    {
        ESP_LOGE("ESPClass", "Partition could not be found");

        return String();
    }
    const size_t bufSize = SPI_FLASH_SEC_SIZE;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[bufSize]);
    uint32_t offset = 0;
    if (!buf.get())
    {
        ESP_LOGE("ESPClass", "Not enough memory to allocate buffer");

        return String();
    }
    MD5Builder md5;
    md5.begin();
    while (lengthLeft > 0)
    {
        size_t readBytes = (lengthLeft < bufSize) ? lengthLeft : bufSize;
        if (!ESP.flashRead(running->address + offset, reinterpret_cast<uint32_t *>(buf.get()), (readBytes + 3) & ~3))
        {
            ESP_LOGE("ESPClass", "Could not read buffer from flash");

            return String();
        }
        md5.add(buf.get(), readBytes);
        lengthLeft -= readBytes;
        offset += readBytes;
    }
    md5.calculate();
    result = md5.toString();
    return result;
}

uint32_t EspClass::getFreeSketchSpace()
{
    const esp_partition_t *_partition = esp_ota_get_next_update_partition(NULL);
    if (!_partition)
    {
        return 0;
    }

    return _partition->size;
}

uint8_t EspClass::getChipRevision(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.revision;
}

uint8_t EspClass::getChipCores(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.cores;
}

const char *EspClass::getSdkVersion(void)
{
    return esp_get_idf_version();
}

uint32_t EspClass::getFlashChipSize(void)
{
    esp_image_header_t fhdr;
    if (flashRead(ESP_FLASH_IMAGE_BASE, (uint32_t *)&fhdr, sizeof(esp_image_header_t)) && fhdr.magic != ESP_IMAGE_HEADER_MAGIC)
    {
        return 0;
    }
    return magicFlashChipSize(fhdr.spi_size);
}

uint32_t EspClass::getFlashChipSpeed(void)
{
    esp_image_header_t fhdr;
    if (flashRead(ESP_FLASH_IMAGE_BASE, (uint32_t *)&fhdr, sizeof(esp_image_header_t)) && fhdr.magic != ESP_IMAGE_HEADER_MAGIC)
    {
        return 0;
    }
    return magicFlashChipSpeed(fhdr.spi_speed);
}

FlashMode_t EspClass::getFlashChipMode(void)
{
    esp_image_header_t fhdr;
    if (flashRead(ESP_FLASH_IMAGE_BASE, (uint32_t *)&fhdr, sizeof(esp_image_header_t)) && fhdr.magic != ESP_IMAGE_HEADER_MAGIC)
    {
        return FM_UNKNOWN;
    }
    return magicFlashChipMode(fhdr.spi_mode);
}

uint32_t EspClass::magicFlashChipSize(uint8_t byte)
{
    switch (byte & 0x0F)
    {
    case 0x0: // 8 MBit (1MB)
        return (1_MB);
    case 0x1: // 16 MBit (2MB)
        return (2_MB);
    case 0x2: // 32 MBit (4MB)
        return (4_MB);
    case 0x3: // 64 MBit (8MB)
        return (8_MB);
    case 0x4: // 128 MBit (16MB)
        return (16_MB);
    default: // fail?
        return 0;
    }
}

uint32_t EspClass::magicFlashChipSpeed(uint8_t byte)
{
    switch (byte & 0x0F)
    {
    case 0x0: // 40 MHz
        return (40_MHz);
    case 0x1: // 26 MHz
        return (26_MHz);
    case 0x2: // 20 MHz
        return (20_MHz);
    case 0xf: // 80 MHz
        return (80_MHz);
    default: // fail?
        return 0;
    }
}

FlashMode_t EspClass::magicFlashChipMode(uint8_t byte)
{
    FlashMode_t mode = (FlashMode_t)byte;
    if (mode > FM_SLOW_READ)
    {
        mode = FM_UNKNOWN;
    }
    return mode;
}

bool EspClass::flashEraseSector(uint32_t sector)
{
    return spi_flash_erase_sector(sector) == ESP_OK;
}

// Warning: These functions do not work with encrypted flash
bool EspClass::flashWrite(uint32_t offset, uint32_t *data, size_t size)
{
    return spi_flash_write(offset, (uint32_t *)data, size) == ESP_OK;
}

bool EspClass::flashRead(uint32_t offset, uint32_t *data, size_t size)
{
    return spi_flash_read(offset, (uint32_t *)data, size) == ESP_OK;
}

bool EspClass::partitionEraseRange(const esp_partition_t *partition, uint32_t offset, size_t size)
{
    return esp_partition_erase_range(partition, offset, size) == ESP_OK;
}

bool EspClass::partitionWrite(const esp_partition_t *partition, uint32_t offset, uint32_t *data, size_t size)
{
    return esp_partition_write(partition, offset, data, size) == ESP_OK;
}

bool EspClass::partitionRead(const esp_partition_t *partition, uint32_t offset, uint32_t *data, size_t size)
{
    return esp_partition_read(partition, offset, data, size) == ESP_OK;
}

uint64_t EspClass::getEfuseMac(void)
{
    uint64_t _chipmacid = 0LL;
    esp_efuse_mac_get_default((uint8_t *)(&_chipmacid));
    return _chipmacid;
}