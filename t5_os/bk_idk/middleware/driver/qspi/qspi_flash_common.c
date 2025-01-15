/*
 * qspi_flash_common.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include <string.h>
#include "driver/qspi_flash_common.h"


static qspi_driver_desc_t *qspi_flash_devices[] = {
    &qspi_w25q_desc,
    &qspi_gd5f1g_desc,
    &qspi_gd25q127c_desc,
};

qspi_driver_desc_t *tuya_qspi_device_query(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    for (int i = 0; i < sizeof(qspi_flash_devices)/sizeof(qspi_driver_desc_t *); i++) {
        if (0 == strcmp(name, qspi_flash_devices[i]->name)) {
            return qspi_flash_devices[i];
        }
    }

    bk_printf("not found qspi flash: %s", name);
    return NULL;
}


