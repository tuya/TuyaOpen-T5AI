// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <components/system.h>
#include <os/os.h>
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/int.h>
#include <os/mem.h>
#include <driver/qspi.h>
#include "qspi_driver.h"
#include "qspi_hal.h"
#include "qspi_statis.h"
#include <driver/trng.h>
#include "driver/qspi_flash_common.h"

#define QSPI_QUAD_ENABLE    1

/* GD25 Instructions *****************************************************************/

/*      Command                         Value     Description       Addr  Dummy     Data    */

#define GD25_WRITE_ENABLE               0x06 /*                     0       0       0     */
#define GD25_WRITE_DISABLE              0x04 /*                     0       0       0     */
#define GD25_READ_STATUS_REGISTER1      0x05 /* Read status register-1
                                              *                     1       0       1     */
#define GD25_READ_STATUS_REGISTER2      0x35 /* Read status register-2
                                              *                     1       0       1     */
#define GD25_READ_STATUS_REGISTER3      0x15 /* Read status register-3
                                              *                     1       0       1     */
#define GD25_WRITE_STATUS_REGISTER1     0x01 /* Write status register-1
                                              *                     1       0       1     */
#define GD25_WRITE_STATUS_REGISTER2     0x31 /* Write status register-2
                                              *                     1       0       1     */
#define GD25_WRITE_STATUS_REGISTER3     0x11 /* Write status register-3
                                              *                     1       0       1     */
#define GD25_READ_DATA                  0x03 /* Read data           3       0       1-2112 */
#define GD25_QUAD_OUTPUT_FAST_READ      0xEB /* Read data
                                              *  on SIO 0/1/2/3    3       1
                                              *  (Quad Output)      1-2112 */

#define GD25_READ_ID                    0x9F /* Read device ID      0       0       3      */

#define GD25_PAGE_PROGRAM               0x02 /* Load program data with
                                              * cache reset first   2       0       1-2112 */
#define GD25_QUAD_PAGE_PROGRAM          0x32 /* Load program data
                                              * without cache reset 2       0       1-2112 */
#define GD25_BLOCK_ERASE                0xD8 /* 64K Block erase         3       0       0     */
#define GD25_SECTOR_ERASE               0x20 /* 4K Block erase         3       0       0     */
#define GD25_CHIP_ERASE                 0xC7 /* Chip erase              0       0       0     */
#define GD25_ENABLE_RESET               0x66 /* ENABLE Reset        0       0       0     */
#define GD25_RESET                      0x99 /* Reset the device    0       0       0     */

/* Feature register ******************************************************************/

#define GD25_BLOCK_SHIFT            16    /* 65536 byte(64K) */
#define GD25_PAGE_SHIFT             8    /* 256 */
#define GD25_PAGE_MASK              ((1 << GD25_PAGE_SHIFT) - 1)
#define GD25_BLOCK_MASK             ((1 << GD25_BLOCK_SHIFT) - 1)
#define GD25_PAGE_SIZE              (1 << GD25_PAGE_SHIFT)
#define GD25_BLOCK_SIZE             (1 << GD25_BLOCK_SHIFT)

/* Bit definitions */

/* Status register */

/* Register1 */
#define GD25_SR_WIP                 (1 << 0)
#define GD25_SR_WEL                 (1 << 1)
#define GD25_SR_BP0                 (1 << 2)
#define GD25_SR_BP1                 (1 << 3)
#define GD25_SR_BP2                 (1 << 4)
#define GD25_SR_BP3                 (1 << 5)
#define GD25_SR_BP4                 (1 << 6)
#define GD25_SR_SRP0                (1 << 7)

/* Register2 */
#define GD25_SR_SRP1                (1 << 0)
#define GD25_SR_QE                  (1 << 1)
#define GD25_SR_CMP                 (1 << 6)

#define FLASH_PAGE_SIZE            0x100
#define FLASH_PAGE_MASK            (FLASH_PAGE_SIZE - 1)
#define FLASH_SECTOR_SIZE          0x1000

#define QSPI_FIFO_LEN_MAX          256
#define FLASH_PROTECT_NONE_DATA    0

static void bk_qspi_flash_gd25_wait_wip_done(qspi_id_t id);

static bk_err_t bk_qspi_flash_gd25_init(void)
{
    // 11 ~ 12 MHz
    qspi_config_t config = {0};
    config.src_clk = QSPI_SCLK_320M;
    config.src_clk_div = 0x6;
    config.clk_div = 0x4;
    BK_LOG_ON_ERR(bk_qspi_init(1, &config));
    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_deinit(void)
{
    BK_LOG_ON_ERR(bk_qspi_deinit(QSPI_ID_1));
    return BK_OK;
}

static uint32_t bk_qspi_flash_gd25_read_status(qspi_id_t id, uint32_t cmd)
{
    qspi_cmd_t read_status_cmd = {0};
    uint32_t status_reg_data = 0;

    read_status_cmd.device = QSPI_FLASH;
    read_status_cmd.wire_mode = QSPI_1WIRE;
    read_status_cmd.work_mode = INDIRECT_MODE;
    read_status_cmd.op = QSPI_READ;
    read_status_cmd.cmd = cmd;

    read_status_cmd.data_len = 1;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
    bk_qspi_read(id, &status_reg_data, 1);

    return status_reg_data & 0xff;
}

static void bk_qspi_flash_gd25_wait_wip_done(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    for(int i = 0; i <= 2000; i++) {
        status_reg_data = bk_qspi_flash_gd25_read_status(id, GD25_READ_STATUS_REGISTER1);
        if(0 == (status_reg_data & GD25_SR_WIP)) {
            break;
        }
        if(i == 2000) {
            QSPI_LOGW("[%s]: wait flsh progress done timeout.\n", __func__);
        }
        rtos_delay_milliseconds(1);
    }
}

static void bk_qspi_flash_gd25_write_enable(qspi_id_t id)
{
    qspi_cmd_t wren_cmd = {0};

    wren_cmd.device = QSPI_FLASH;
    wren_cmd.wire_mode = QSPI_1WIRE;
    wren_cmd.work_mode = INDIRECT_MODE;
    wren_cmd.op = QSPI_WRITE;
    wren_cmd.cmd = GD25_WRITE_ENABLE;
    wren_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &wren_cmd));
    bk_qspi_flash_gd25_wait_wip_done(id);
}

static bk_err_t bk_qspi_flash_gd25_write_s0_s7(qspi_id_t id, uint8_t status_reg_data)
{
    qspi_cmd_t write_status_cmd = {0};

    bk_qspi_flash_gd25_write_enable(id);

    write_status_cmd.device = QSPI_FLASH;
    write_status_cmd.wire_mode = QSPI_1WIRE;
    write_status_cmd.work_mode = INDIRECT_MODE;
    write_status_cmd.op = QSPI_WRITE;
    write_status_cmd.cmd = (status_reg_data << 8) | GD25_WRITE_STATUS_REGISTER1;

    BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
    bk_qspi_flash_gd25_wait_wip_done(id);

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_write_s8_s15(qspi_id_t id, uint8_t status_reg_data)
{
    qspi_cmd_t write_status_cmd = {0};

    bk_qspi_flash_gd25_write_enable(id);

    write_status_cmd.device = QSPI_FLASH;
    write_status_cmd.wire_mode = QSPI_1WIRE;
    write_status_cmd.work_mode = INDIRECT_MODE;
    write_status_cmd.op = QSPI_WRITE;
    write_status_cmd.cmd = (status_reg_data << 8) | GD25_WRITE_STATUS_REGISTER2;

    BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
    bk_qspi_flash_gd25_wait_wip_done(id);

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_write_s16_s23(qspi_id_t id, uint8_t status_reg_data)
{
    qspi_cmd_t write_status_cmd = {0};

    bk_qspi_flash_gd25_write_enable(id);

    write_status_cmd.device = QSPI_FLASH;
    write_status_cmd.wire_mode = QSPI_1WIRE;
    write_status_cmd.work_mode = INDIRECT_MODE;
    write_status_cmd.op = QSPI_WRITE;
    write_status_cmd.cmd = GD25_WRITE_STATUS_REGISTER3;
    write_status_cmd.addr = status_reg_data;
    write_status_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT8;

    BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
    bk_qspi_flash_gd25_wait_wip_done(id);

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_quad_enable(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)bk_qspi_flash_gd25_read_status(id, GD25_READ_STATUS_REGISTER2);
    if ((status_reg_data & GD25_SR_QE) == 0) {
        status_reg_data |= GD25_SR_QE;
        return bk_qspi_flash_gd25_write_s8_s15(id, status_reg_data);
    }
    return BK_OK;
}

static void bk_qspi_flash_gd25_quad_disable(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)bk_qspi_flash_gd25_read_status(id, GD25_READ_STATUS_REGISTER2);
    if ((status_reg_data & GD25_SR_QE) == GD25_SR_QE) {
        status_reg_data &= ~GD25_SR_QE;
        bk_qspi_flash_gd25_write_s8_s15(id, status_reg_data);
    }
}

static bk_err_t bk_qspi_flash_gd25_erase_block(uint32_t addr)
{
    qspi_cmd_t erase_block_cmd = {0};


//    bk_printf("===> entry %s\r\n", __func__);
    // bk_printf("qspi flash erase block, ra: %08x, block: %04x\r\n", ra, block_num);
    bk_qspi_flash_gd25_write_enable(QSPI_ID_1);

    erase_block_cmd.device = QSPI_FLASH;
    erase_block_cmd.wire_mode = QSPI_1WIRE;
    erase_block_cmd.work_mode = INDIRECT_MODE;
    erase_block_cmd.op = QSPI_WRITE;
    erase_block_cmd.cmd = GD25_BLOCK_ERASE;
    erase_block_cmd.addr = addr;
    erase_block_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;

    BK_LOG_ON_ERR(bk_qspi_command(QSPI_ID_1, &erase_block_cmd));
    bk_qspi_flash_gd25_wait_wip_done(QSPI_ID_1);

//    bk_printf("<=== exit %s\r\n\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd5f_standrad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
//    bk_printf("===> entry %s\r\n", __func__);
    if (data == NULL || size == 0)
        return -1;

    // 1. issue write date to cache cmd <02h/32h>
    bk_qspi_flash_gd5f_std_write_to_cache(id, addr, data, size);

    // 2. issue write enable cmd <06h>
    bk_qspi_flash_gd25_write_enable(id);

    // 3. issue program execute cmd <10h> + 24-bit address
    // the 24-bit address is actual address to be written to flash
    bk_qspi_flash_gd5f_execute_write(id, addr);
//    bk_printf("<=== exit %s\r\n\r\n", __func__);

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_page_program(qspi_id_t id, uint32_t addr, const void *data, int16_t size)
{
    qspi_cmd_t cache_program_cmd = {0};
    uint32_t aligned_address = addr & (~GD25_PAGE_MASK);
    uint16_t len = 0;
    uint8_t *buffer = (uint8_t *)data;

    cache_program_cmd.device = QSPI_FLASH;
    cache_program_cmd.wire_mode = QSPI_1WIRE;
    cache_program_cmd.work_mode = INDIRECT_MODE;
    cache_program_cmd.op = QSPI_WRITE;
    cache_program_cmd.cmd = GD25_PAGE_PROGRAM;
    cache_program_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;

    while(0 < size) {
        bk_qspi_flash_gd25_write_enable(id);
        len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        bk_qspi_write(id, buffer, len);
        cache_program_cmd.addr = aligned_address;
        cache_program_cmd.data_len = len;
        BK_LOG_ON_ERR(bk_qspi_command(id, &cache_program_cmd));

        aligned_address += len;
        buffer += len;
        size -= len;
        bk_qspi_flash_gd25_wait_wip_done(id);
    }

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, int16_t size)
{
    qspi_cmd_t quad_page_program_cmd = {0};
    uint32_t aligned_address = addr & (~GD25_PAGE_MASK);
    uint16_t len = 0;
    uint8_t *buffer = (uint8_t *)data;

    quad_page_program_cmd.device = QSPI_FLASH;
    quad_page_program_cmd.wire_mode = QSPI_4WIRE;
    quad_page_program_cmd.work_mode = INDIRECT_MODE;
    quad_page_program_cmd.op = QSPI_WRITE;
    quad_page_program_cmd.cmd = GD25_QUAD_PAGE_PROGRAM;
    quad_page_program_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;

    while(0 < size) {
        bk_qspi_flash_gd25_write_enable(id);
        len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        bk_qspi_write(id, buffer, len);
        quad_page_program_cmd.addr = aligned_address;
        quad_page_program_cmd.data_len = len;
        BK_LOG_ON_ERR(bk_qspi_command(id, &quad_page_program_cmd));

        aligned_address += len;
        buffer += len;
        size -= len;
        bk_qspi_flash_gd25_wait_wip_done(id);
    }

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_quad_write_page(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
    if (data == NULL || size == 0)
        return -1;

//    bk_printf("===> entry %s\r\n", __func__);
    bk_qspi_flash_gd25_quad_enable(id);
    bk_qspi_flash_gd25_quad_page_program(id, addr, data, size);
    // bk_qspi_flash_gd25_quad_disable(id);
//    bk_printf("<=== exit %s\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_read_data(qspi_id_t id, uint32_t addr, void *data, int32_t size)
{
    qspi_cmd_t read_data_cmd = {0};
    uint8_t *buffer = data;
    uint32_t aligned_address = addr & (~GD25_PAGE_MASK);
    int16_t len = 0;

    if (buffer == NULL || size == 0)
        return BK_FAIL;

    read_data_cmd.device = QSPI_FLASH;
    read_data_cmd.wire_mode = QSPI_1WIRE;
    read_data_cmd.work_mode = INDIRECT_MODE;
    read_data_cmd.op = QSPI_READ;
    read_data_cmd.cmd = GD25_READ_DATA;

    while(0 < size) {
        len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        read_data_cmd.addr = aligned_address;
        read_data_cmd.data_len = len;
        read_data_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;
        BK_LOG_ON_ERR(bk_qspi_command(id, &read_data_cmd));
        bk_qspi_read(id, buffer, len);

        aligned_address += len;
        buffer += len;
        size -= len;
    }

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_quad_fast_read(qspi_id_t id, uint32_t addr, void *data, int32_t size)
{
    qspi_cmd_t read_data_cmd = {0};
    uint8_t *buffer = data;
    uint32_t aligned_address = addr & (~GD25_PAGE_MASK);
    int16_t len = 0;

    if (buffer == NULL)
        return -1;

    read_data_cmd.device = QSPI_FLASH;
    read_data_cmd.wire_mode = QSPI_4WIRE;
    read_data_cmd.work_mode = INDIRECT_MODE;
    read_data_cmd.op = QSPI_READ;
    read_data_cmd.cmd = GD25_QUAD_OUTPUT_FAST_READ;
    read_data_cmd.dummy_cycle = 4;

    while(0 < size) {
        len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        read_data_cmd.addr = aligned_address << 8;
        read_data_cmd.data_len = len;
        read_data_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT32;
        BK_LOG_ON_ERR(bk_qspi_command(id, &read_data_cmd));
        bk_qspi_read(id, buffer, len);

        aligned_address += len;
        buffer += len;
        size -= len;
    }

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_quad_page_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
//    bk_printf("===> entry %s\r\n", __func__);
    if (data == NULL || size == 0)
        return BK_FAIL;

    bk_qspi_flash_gd25_quad_enable(id);
    bk_qspi_flash_gd25_quad_fast_read(id, addr, data, size);
    // bk_qspi_flash_gd25_quad_disable(1);
//    bk_printf("<=== exit %s\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd25_set_protect_none(void)
{
    uint8_t status_reg_data = 0;

    status_reg_data = bk_qspi_flash_gd25_read_status(QSPI_ID_1, GD25_READ_STATUS_REGISTER1) & 0xff;
    uint8_t clean_bits = ~(GD25_SR_BP0 | GD25_SR_BP1 | GD25_SR_BP2);
    status_reg_data &= clean_bits;
    bk_qspi_flash_gd25_write_s0_s7(QSPI_ID_1, status_reg_data);
    return BK_OK;
}

static uint32_t bk_qspi_flash_gd25_read_id(void)
{
    qspi_cmd_t read_id_cmd = {0};
    uint32_t read_id_data = 0;

    read_id_cmd.device = QSPI_FLASH;
    read_id_cmd.wire_mode = QSPI_1WIRE;
    read_id_cmd.work_mode = INDIRECT_MODE;
    read_id_cmd.op = QSPI_READ;
    read_id_cmd.cmd = GD25_READ_ID;
    // read_id_cmd.addr = 0x00;
    // read_id_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT8;
    read_id_cmd.data_len = 3;

    BK_LOG_ON_ERR(bk_qspi_command(1, &read_id_cmd));

    bk_qspi_read(1, &read_id_data, 3);

    return read_id_data;
}

static bk_err_t bk_qspi_flash_gd25_read_page(uint32_t addr, void *data, uint32_t size)
{

#if (QSPI_QUAD_ENABLE == 1)
    return bk_qspi_flash_gd25_quad_page_read(1, addr, data, size);
#else // QSPI_QUAD_ENABLE == 0
    return bk_qspi_flash_gd25_read_data(1, addr, data, size);
#endif

}

static bk_err_t bk_qspi_flash_gd25_write_page(uint32_t addr, const void *data, uint32_t size)
{
#if (QSPI_QUAD_ENABLE == 1)
    return bk_qspi_flash_gd25_quad_write_page(1, addr, data, size);
#else // QSPI_QUAD_ENABLE == 0
    return bk_qspi_flash_gd25_page_program(1, addr, data, size);
#endif
}

uint32_t bk_qspi_flash_gd25_read_id_test(qspi_id_t id, uint32_t cmd, uint32_t addr, uint32_t addr_len, uint32_t data_len)
{
    qspi_cmd_t read_id_cmd = {0};
    uint32_t read_id_data = 0;

    read_id_cmd.device = QSPI_FLASH;
    read_id_cmd.wire_mode = QSPI_1WIRE;
    read_id_cmd.work_mode = INDIRECT_MODE;
    read_id_cmd.op = QSPI_READ;
    read_id_cmd.cmd = cmd;
    read_id_cmd.addr = addr;
    read_id_cmd.addr_valid_bit = addr_len;
    read_id_cmd.data_len = data_len;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_id_cmd));

    bk_qspi_read(id, &read_id_data, data_len);

    return read_id_data;
}

#include "tkl_memory.h"
uint32_t bk_qspi_flash_gd25_test(uint32_t test_len)
{
    // uint32_t read_id_data;
    // uint8_t status_reg_data = 0;
    // uint32_t test_len = 5;

    // read_id_data = bk_qspi_flash_gd25_read_id();

    // bk_qspi_flash_gd25_write_s16_s23(QSPI_ID_1, 0x40);

    // status_reg_data = bk_qspi_flash_gd25_read_status(QSPI_ID_1, GD25_READ_STATUS_REGISTER1) & 0xff;
    // bk_printf("status reg1 data: 0x%x\r\n", status_reg_data);
    // status_reg_data = bk_qspi_flash_gd25_read_status(QSPI_ID_1, GD25_READ_STATUS_REGISTER2) & 0xff;
    // bk_printf("status reg2 data: 0x%x\r\n", status_reg_data);
    // status_reg_data = bk_qspi_flash_gd25_read_status(QSPI_ID_1, GD25_READ_STATUS_REGISTER3) & 0xff;
    // bk_printf("status reg3 data: 0x%x\r\n", status_reg_data);

    bk_qspi_flash_gd25_set_protect_none();

    // bk_qspi_flash_gd25_quad_enable(QSPI_ID_1);

    test_len <<= 10;
    bk_printf("test length: %d / %dKB\r\n", test_len, test_len >> 10);

    uint8_t *read_buffer = (uint8_t *)psram_malloc(test_len);
    if (read_buffer == NULL) {
        bk_printf("------- malloc read buffer error ------\r\n");
        return -1;
    }
    os_memset(read_buffer, 0, test_len);

    uint8_t *write_buffer = (uint8_t *)psram_malloc(test_len);
    if (write_buffer == NULL) {
        bk_printf("------- malloc write buffer error ------\r\n");
        tkl_system_free(read_buffer);
        read_buffer = NULL;
        return -1;
    }

    uint32_t c = 0x1;
    for (int i = 0; i < test_len; i++) {
        // c++;
        if (c % 256 == 0)
            c = 0x1;
        write_buffer[i] = c++;
        // bk_printf("write_buffer[%d] = %02x\r\n", i, write_buffer[i]);
    }

    // bk_qspi_flash_gd25_read_data(QSPI_ID_1, 0x21f000, read_buffer, test_len);
    // bk_qspi_flash_gd25_read_page(0x21f000, read_buffer, test_len);

    // for (int i = 0; i < test_len; i++) {
    //     bk_printf("read_buffer[%d] = %02x\r\n", i, read_buffer[i]);
    // }
    // bk_printf("read_buffer end\n");

    bk_printf("erase_buffer start\n");
    bk_qspi_flash_gd25_erase_block(0x21ff00);
    bk_printf("erase_buffer end\n");

    // bk_qspi_flash_gd25_read_data(QSPI_ID_1, 0x21f000, read_buffer, test_len);
    // bk_qspi_flash_gd25_read_page(0x21f000, read_buffer, test_len);

    // for (int i = 0; i < test_len; i++) {
    //     bk_printf("read_buffer[%d] = %02x\r\n", i, read_buffer[i]);
    // }
    // bk_printf("read_buffer end\n");

    bk_qspi_flash_gd25_page_program(QSPI_ID_1, 0x21ff00, write_buffer, test_len);
    // bk_qspi_flash_gd25_quad_write_page(QSPI_ID_1, 0x21ff00, write_buffer, test_len);

    bk_qspi_flash_gd25_read_data(QSPI_ID_1, 0x21ff00, read_buffer, test_len);
    // bk_qspi_flash_gd25_read_page(0x21ff00, read_buffer, test_len);

    for (int i = 0; i < test_len; i++) {
        bk_printf("read_buffer[%d] = %02x\r\n", i, read_buffer[i]);
    }
    bk_printf("read_buffer end\n");

    for (int i = 0; i < test_len; i++) {
        if (write_buffer[i] != read_buffer[i]) {
            bk_printf("!!!!!!! error  %d %02x %02x !!!!!!\r\n", i, write_buffer[i], read_buffer[i]);
            break;
        }
    }

    return 0;
}

qspi_driver_desc_t qspi_gd25q127c_desc = {
    .name = "gd25q127c",
    .page_size = GD25_PAGE_SIZE,
    .block_size = GD25_BLOCK_SIZE,
    .total_size = (64 * 256 * 1024),        // 16MByte

    .init = bk_qspi_flash_gd25_init,
    .deinit = bk_qspi_flash_gd25_deinit,
    .read_id = bk_qspi_flash_gd25_read_id,
    .unblock = bk_qspi_flash_gd25_set_protect_none,
    .read_page = bk_qspi_flash_gd25_read_page,
    .write_page = bk_qspi_flash_gd25_write_page,
    .erase_block = bk_qspi_flash_gd25_erase_block,
};


