/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2022 Arm Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include "boot_hal.h"
#include "sdkconfig.h"
#include "partitions_gen.h"
#include "flash_partition.h"
#include "aon_pmu_hal.h"
#include <modules/pm.h>
#include "cmsis.h"
#include "flash_layout.h"
#include "sys_hal.h"

/* Avoids the semihosting issue */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

#define HDR_SZ                  0x1000

#define addSYSTEM_Reg0xc                    *((volatile unsigned long *) (0x44010000+0xc*4))
#define SPI0_ENABLE                         addSYSTEM_Reg0xc |= 0x2
#define addSYSTEM_Reg0x2                    *((volatile unsigned long *) (0x44010000+0x2*4))
#define SPI_FLASH_SEL                       (0x1U << 9 )
#define SET_SPI_RW_FLASH                    addSYSTEM_Reg0x2 |= SPI_FLASH_SEL
#define SET_FLASHCTRL_RW_FLASH              addSYSTEM_Reg0x2 &= ~SPI_FLASH_SEL

#define CONFIG_OTA_VIRTUAL_CODE_OFFSET      FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_OTA_PHY_PARTITION_OFFSET))
#define _CTRL_CTRL_MAGIC (0x4C725463)

typedef enum boot_flag_t {
    BOOT_FLAG_INVALID  	= 0,
    BOOT_FLAG_PRIMARY  	= 1,
    BOOT_FLAG_SECONDARY = 2,
}BOOT_FLAG;

typedef struct _boot_ctrl_data_t {
    uint32_t magic;
    uint32_t boot_flag;
} boot_ctrl_data_t;

static void bl_djump_to_bootloader(uint32_t addr)
{
    uint32_t msp = 0;
    uint32_t pc  = 0;

    msp = *(uint32_t *)addr;
    pc  = *(uint32_t *)(addr + 4);

    /* relocate vecotr table */
    SCB->VTOR = (addr);
    __DSB();
    __ISB();

    /* save next entry to r9 */
    __asm__ volatile("mov r9, %0" : : "r"(pc) : "memory");

    __set_MSP(msp);
    __DSB();
    __ISB();

    __asm__ volatile(
        "bx r9;\n\t"
        "b .;\n\t"      /* should not be here */
        );
}

void deep_sleep_reset(void)
{
    if (sys_is_running_from_deep_sleep() && sys_is_enable_fast_boot() && (!sys_is_running_from_ota())) {
        bl_djump_to_bootloader(FLASH_BASE_ADDRESS + CONFIG_BL2_VIRTUAL_CODE_START);
    }
}

static int32_t hal_read_preferred_boot_flag(BOOT_FLAG *flag)
{
    int32_t ret = 0;
    boot_ctrl_data_t data = {0};

    if (!flag) {
        // printf("Parameter flag is NULL!\n");
        return -1;
    }
    SET_SPI_RW_FLASH;
    ret = ext_flash_rd_data(CONFIG_BOOT_FLAG_PHY_PARTITION_OFFSET,
                        (uint8_t *)(&data),
                        sizeof(boot_ctrl_data_t));
    if (0 != ret) {
        // printf("hal flash read failed!\n");
        return -1;
    }
    SET_FLASHCTRL_RW_FLASH;
    if (data.magic == _CTRL_CTRL_MAGIC) {
        if ((data.boot_flag == BOOT_FLAG_PRIMARY) ||
            (data.boot_flag == BOOT_FLAG_SECONDARY)) {
            *flag = data.boot_flag;
        } else {
            *flag = BOOT_FLAG_PRIMARY;
        }
    } else {
        *flag = BOOT_FLAG_PRIMARY;
    }

    return ret;
}

static uint32_t bl_get_bootloader_jump_addr(void)
{
    BOOT_FLAG boot_flag = BOOT_FLAG_PRIMARY;
    uint32_t addr = 0;

    hal_read_preferred_boot_flag(&boot_flag);

    if(boot_flag == BOOT_FLAG_PRIMARY)
    {
        addr = CONFIG_BL2_VIRTUAL_CODE_START;
    }
    else
    {
        addr = CONFIG_OTA_VIRTUAL_CODE_OFFSET;
    }
    addr += FLASH_BASE_ADDRESS ;

    return addr;
}

int main(void)
{
    uint32_t addr = 0;
    SPI0_ENABLE;
    spi_initial(1,512000);
    addr = bl_get_bootloader_jump_addr();
    bl_djump_to_bootloader(addr);
    return 0;
}

