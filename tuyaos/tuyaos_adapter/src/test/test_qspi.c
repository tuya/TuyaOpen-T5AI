/*
 * test_qspi.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "stdint.h"
#include "tuya_cloud_types.h"

#include "cli.h"
#include <driver/int_types.h>
#include "cli_tuya_test.h"
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/qspi.h>
#include <driver/qspi_flash_common.h>

static volatile int __qspi_has_inited = 0;

void cli_xqspi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int i;
    uint32_t c = 0x1;
    uint32_t test_len = 200;
    uint32_t addr = 0x21f000;

    bk_printf("argc: %d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    qspi_driver_desc_t *qflash_dev = NULL;
    qflash_dev = tuya_qspi_device_query(CONFIG_TUYA_QSPI_FLASH_TYPE);
    if (qflash_dev == NULL) {
        bk_printf("Not found qspi flash %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
        return BK_FAIL;
    }

    if (__qspi_has_inited == 0) {
        qflash_init();
        __qspi_has_inited = 1;
    }

    if (argc >= 2) {
        if (!strcmp("aa", argv[1])) {
            c = os_strtoul(argv[2], NULL, 10);
        } else if (!strcmp("fce", argv[1])) {
            // full chip erase
            qflash_erase(0, qflash_dev->total_size);
            return;
        } else {
            test_len = os_strtoul(argv[1], NULL, 10);
        }
    }

    test_len <<= 10;
    test_len += qflash_dev->block_size;
    test_len /= qflash_dev->block_size;
    test_len *= qflash_dev->block_size;

    bk_printf("\r\n------- qspi test init ------\r\n");

    bk_printf("\r\n--- %d\r\n", test_len);
    uint8_t *write_buffer = (uint8_t *)psram_malloc(test_len);
    if (write_buffer == NULL) {
        bk_printf("------- qspi test 3 failed, malloc write buffer error ------\r\n");
        return;
    }

    uint8_t *read_buffer = (uint8_t *)psram_malloc(test_len);
    if (read_buffer == NULL) {
        bk_printf("------- qspi test 3 failed, malloc read buffer error ------\r\n");
        tkl_system_free(write_buffer);
        write_buffer = NULL;
        return;
    }

    bk_printf("\r\n------- qspi test read id ------\r\n");

    uint32_t v = qflash_dev->read_id();
    bk_printf("flash id: 0x%03x\r\n", v & 0xFFFFFF);

#if 1
    bk_printf("------- qspi test erase ------\r\n");

    qflash_dev->unblock();
    qflash_erase(addr, test_len);

    bk_printf("------- qspi large test ------\r\n");

    bk_printf("test write length: %d / %dKB\r\n", test_len, test_len >> 10);
    for (int i = 0; i < test_len; i++) {
        write_buffer[i] = c + (i / 0xff) & 0xff;
    }

    qflash_write(addr, write_buffer, test_len);

    // tkl_system_sleep(10);

    bk_printf("------- read ------\r\n");

    // bk_qspi_read(id, data, size);
    memset(read_buffer, 0x5a, test_len);
    qflash_read(addr, read_buffer, test_len);

    bk_printf("------- data check ------\r\n");
    for (i = 0; i < test_len; i++) {
        if (write_buffer[i] != read_buffer[i]) {
            // bk_printf("!!!!!!! error  %d %02x %02x !!!!!!\r\n", i, write_buffer[i], read_buffer[i]);
            bk_printf("!!!!!!! error  %d %x %x !!!!!!\r\n", i, write_buffer[i], read_buffer[i]);
            break;
        }
    }

#endif

    bk_printf("------- qspi test end ------\r\n");

    psram_free(write_buffer);
    write_buffer = NULL;

    psram_free(read_buffer);
    read_buffer = NULL;

    return;
}

