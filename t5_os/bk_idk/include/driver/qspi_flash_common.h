/*
 * qspi_flash_common.h
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/int.h>

#ifndef QSPI_FLASH_COMMON_H
#define QSPI_FLASH_COMMON_H

// TODO FLASH LOCK
typedef bk_err_t (*QFLASH_INIT_T)(void);
typedef bk_err_t (*QFLASH_DEINIT_T)(void);
typedef uint32_t (*QFLASH_READ_ID_T)(void);
typedef bk_err_t (*QFLASH_UNLOCK_T)(void);
typedef bk_err_t (*QFLASH_PAGE_READ_T)(uint32_t addr, void *data, uint32_t size);
typedef bk_err_t (*QFLASH_PAGE_WRITE_T)(uint32_t addr, const void *data, uint32_t size);
typedef bk_err_t (*QFLASH_BLOCK_ERASE_T)(uint32_t addr);

typedef struct {
    const char *name;

    uint32_t page_size;
    uint32_t block_size;
    uint32_t total_size;

    QFLASH_INIT_T           init;
    QFLASH_DEINIT_T         deinit;
    QFLASH_READ_ID_T        read_id;
    QFLASH_UNLOCK_T         unblock;
    QFLASH_PAGE_READ_T      read_page;
    QFLASH_PAGE_WRITE_T     write_page;
    QFLASH_BLOCK_ERASE_T    erase_block;

} qspi_driver_desc_t;

extern qspi_driver_desc_t qspi_w25q_desc;
extern qspi_driver_desc_t qspi_gd5f1g_desc;
extern qspi_driver_desc_t qspi_gd25q127c_desc;
qspi_driver_desc_t *tuya_qspi_device_query(const char *name);

#endif /* !QSPI_FLASH_COMMON_H */

