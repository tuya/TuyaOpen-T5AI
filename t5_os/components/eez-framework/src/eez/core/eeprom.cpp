/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <eez/conf-internal.h>

#if !defined(EEZ_FOR_LVGL)

#include <string.h>

#include <eez/core/eeprom.h>
#include <eez/core/os.h>
#include <eez/core/util.h>

//#include <bb3/system.h>
//#include <bb3/psu/psu.h>

#if defined(EEZ_PLATFORM_STM32) && !CONF_SURVIVE_MODE
#define USE_EEPROM 1
#else
#define USE_EEPROM 0
#endif

#if defined(EEZ_PLATFORM_STM32)
#ifdef EEZ_PLATFORM_STM32F469I_DISCO
#else
#include <i2c.h>
#endif
#endif

#if !USE_EEPROM
#include <eez/fs/fs.h>
#endif

#if OPTION_SCPI
#include <scpi/scpi.h>
#endif

namespace eez {
namespace eeprom {

#if defined(EEZ_PLATFORM_STM32)
// opcodes
static const uint8_t WREN = 6;
static const uint8_t WRDI = 4;
static const uint8_t RDSR = 5;
static const uint8_t WRSR = 1;
static const uint8_t READ = 3;
static const uint8_t WRITE = 2;

// EEPROM AT24C256C
// I2C-Compatible (2-Wire) Serial EEPROM
// 256-Kbit (32,768 x 8)
// http://ww1.microchip.com/downloads/en/devicedoc/atmel-8568-seeprom-at24c256c-datasheet.pdf
#ifndef EEPROM_I2C_ADDRESS
#define EEPROM_I2C_ADDRESS (0x50 << 1)
#endif

#ifndef EEPROM_I2C_HANDLE
#define EEPROM_I2C_HANDLE hi2c1
#endif

#endif

TestResult g_testResult = TEST_FAILED;

////////////////////////////////////////////////////////////////////////////////

#if defined(EEZ_PLATFORM_STM32)

const int MAX_READ_CHUNK_SIZE = 16;
const int MAX_WRITE_CHUNK_SIZE = 16;

bool readFromEEPROM(uint8_t *buffer, uint16_t bufferSize, uint16_t address) {
#ifdef EEZ_PLATFORM_STM32F469I_DISCO
    return false;
#else
    for (uint16_t i = 0; i < bufferSize; i += MAX_READ_CHUNK_SIZE) {
        uint16_t chunkAddress = address + i;

        uint16_t chunkSize = MIN(MAX_READ_CHUNK_SIZE, bufferSize - i);

        uint8_t data[2] = {
                I2C_MEM_ADD_MSB(chunkAddress),
                I2C_MEM_ADD_LSB(chunkAddress)
        };

        HAL_StatusTypeDef returnValue;

        vTaskSuspendAll();
        returnValue = HAL_I2C_Master_Transmit(&EEPROM_I2C_HANDLE, EEPROM_I2C_ADDRESS, data, 2, HAL_MAX_DELAY);
        if (returnValue != HAL_OK) {
            xTaskResumeAll();
            return false;
        }
        returnValue = HAL_I2C_Master_Receive(&EEPROM_I2C_HANDLE, EEPROM_I2C_ADDRESS, buffer + i, chunkSize, HAL_MAX_DELAY);
        xTaskResumeAll();
        if (returnValue != HAL_OK) {
            return false;
        }
    }

    return true;
#endif
}

bool writeToEEPROM(const uint8_t *buffer, uint16_t bufferSize, uint16_t address) {
#ifdef EEZ_PLATFORM_STM32F469I_DISCO
    return false;
#else
    for (uint16_t i = 0; i < bufferSize; i += MAX_WRITE_CHUNK_SIZE) {
        uint16_t chunkAddress = address + i;

        uint16_t chunkSize = MIN(MAX_WRITE_CHUNK_SIZE, bufferSize - i);

        HAL_StatusTypeDef returnValue;

        vTaskSuspendAll();
        returnValue = HAL_I2C_Mem_Write(&EEPROM_I2C_HANDLE, EEPROM_I2C_ADDRESS, chunkAddress, I2C_MEMADD_SIZE_16BIT, (uint8_t *)buffer + i, chunkSize, HAL_MAX_DELAY);
        xTaskResumeAll();

        if (returnValue != HAL_OK) {
            return false;
        }

        osDelay(5);

        // verify
        uint8_t verify[MAX_WRITE_CHUNK_SIZE];

        uint8_t data[2] = {
                I2C_MEM_ADD_MSB(chunkAddress),
                I2C_MEM_ADD_LSB(chunkAddress)
        };

        vTaskSuspendAll();
        returnValue = HAL_I2C_Master_Transmit(&EEPROM_I2C_HANDLE, EEPROM_I2C_ADDRESS, data, 2, HAL_MAX_DELAY);
        if (returnValue != HAL_OK) {
            xTaskResumeAll();
            return false;
        }
        returnValue = HAL_I2C_Master_Receive(&EEPROM_I2C_HANDLE, EEPROM_I2C_ADDRESS, verify, chunkSize, HAL_MAX_DELAY);
        xTaskResumeAll();
        if (returnValue != HAL_OK) {
            return false;
        }

        for (int j = 0; j < chunkSize; ++j) {
            if (buffer[i+j] != verify[j]) {
                return false;
            }
        }
    }

    return true;
#endif
}

#endif

#if !USE_EEPROM
const char *EEPROM_FILE_PATH = "/EEPROM.state";
#endif

bool read(uint8_t *buffer, uint16_t bufferSize, uint16_t address) {
#if USE_EEPROM
    return readFromEEPROM(buffer, bufferSize, address);
#else
    File file;
    if (!file.open(EEPROM_FILE_PATH, FILE_READ)) {
        return false;
    }
    file.seek(address);
    size_t readBytes = file.read(buffer, bufferSize);
    if (readBytes < bufferSize) {
        memset(buffer + readBytes, 0xFF, bufferSize - readBytes);
    }
    file.close();
    return true;
#endif
}

bool write(const uint8_t *buffer, uint16_t bufferSize, uint16_t address) {
#if USE_EEPROM
    return writeToEEPROM(buffer, bufferSize, address);
#else
    File file;
    if (!file.open(EEPROM_FILE_PATH, FILE_OPEN_ALWAYS | FILE_WRITE)) {
        return false;
    }
    file.seek(address);
    file.write(buffer, bufferSize);
    file.close();
    return true;
#endif
}

void init() {
	g_testResult = TEST_OK;
}

bool test() {
    // TODO add test
    return g_testResult == TEST_OK;
}

} // namespace eeprom
} // namespace eez

#endif // !defined(EEZ_FOR_LVGL)
