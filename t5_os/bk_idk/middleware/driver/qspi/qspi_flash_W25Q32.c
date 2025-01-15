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

/* Configuration, Status, Erase, Program Commands ***************************
 *      Command                  Value    Description:                      *
 *                                          Data sequence                   *
 */

#define W25Q_READ_STATUS_1          0x05 /* SRP|SEC|TB |BP2|BP1|BP0|WEL|BUSY       */
#define W25Q_READ_STATUS_2          0x35 /* SUS|CMP|LB3|LB2|LB1|(R)|QE |SRL        */
#define W25Q_READ_STATUS_3          0x15 /* HOLD/RST|DRV1|DRV0|(R)|(R)|WPS|(R)|(R) */

#define W25Q_WRITE_STATUS_1         0x01 /* SRP|SEC|TB |BP2|BP1|BP0|WEL|BUSY       */
#define W25Q_WRITE_STATUS_2         0x31 /* SUS|CMP|LB3|LB2|LB1|(R)|QE |SRL        */
#define W25Q_WRITE_STATUS_3         0x11 /* HOLD/RST|DRV1|DRV0|(R)|(R)|WPS|(R)|(R) */

#define W25Q_WRITE_ENABLE           0x06 /* Write enable                           */
#define W25Q_WRITE_DISABLE          0x04 /* Write disable                          */

#define W25Q_PAGE_PROGRAM           0x02 /* Page Program:                          *
                                        *  0x02 | A23-A16 | A15-A8 | A7-A0 | data */
#define W25Q_QUAD_PAGE_PROGRAM      0x32 /* Quad Page Program:                          *
                                        *  0x32 | A23-A16 | A15-A8 | A7-A0 | data */

#define W25Q_SECTOR_ERASE           0x20 /* Sector Erase (4 kB)                    *
                                               *  0x20 | A23-A16 | A15-A8 | A7-A0       */
#define W25Q_BLOCK_ERASE_32K        0x52 /* Block Erase (32 KB)                    *
                                               *  0x52 | A23-A16 | A15-A8 | A7-A0       */
#define W25Q_BLOCK_ERASE_64K        0xd8 /* Block Erase (64 KB)                    *
                                               *  0xd8 | A23-A16 | A15-A8 | A7-A0       */
#define W25Q_CHIP_ERASE             0x60 /* Chip Erase:                            *
                                               *  0xc7 or 0x60                          */

/* Read Commands ************************************************************
 *      Command                        Value   Description:                 *
 *                                               Data sequence              *
 */

#define W25Q_PAGE_READ              0x03  /* Read:        *
                                               *   0x03 | A23-A16 | A15-A8 | A7-A0 | data...    */
#define W25Q_QUAD_PAGE_READ         0x6b  /* Fast Read Quad I/O:        *
                                               *   0xeb | ADDR | data...    */

/* Reset Commands ***********************************************************
 *      Command                  Value    Description:                      *
 *                                          Data sequence                   *
 */

#define W25Q_RESET_ENABLE           0x66  /* Enable Reset                     */
#define W25Q_DEVICE_RESET           0x99  /* Reset Device                     */

/* ID/Security Commands *****************************************************
 *      Command                  Value    Description:                      *
 *                                            Data sequence                 *
 */
#define W25Q_JEDEC_ID               0x9f  /* JEDEC ID:                        *
                                                 * 0x9f | Manufacturer |            *
                                                 * MemoryType | Capacity            */

/* Flash Manufacturer JEDEC IDs */

#define W25Q_JEDEC_ID_WINBOND       0xef  /* Winbond Serial Flash */

/* W25QXXXJV JEDIC IDs */

#define W25Q016_JEDEC_CAPACITY      0x15  /* W25Q016 (2 MB) memory capacity */
#define W25Q032_JEDEC_CAPACITY      0x16  /* W25Q032 (4 MB) memory capacity */
#define W25Q064_JEDEC_CAPACITY      0x17  /* W25Q064 (8 MB) memory capacity */
#define W25Q128_JEDEC_CAPACITY      0x18  /* W25Q128 (16 MB) memory capacity */
#define W25Q256_JEDEC_CAPACITY      0x19  /* W25Q256 (32 MB) memory capacity */
#define W25Q512_JEDEC_CAPACITY      0x20  /* W25Q512 (64 MB) memory capacity */
#define W25Q01_JEDEC_CAPACITY       0x21  /* W25Q01 (128 MB) memory capacity */

/* W25QXXXJV Registers ******************************************************/

/* Status register 1 bit definitions                                      */

#define STATUS_BUSY_MASK            (1 << 0) /* Bit 0: Device ready/busy status  */
#define STATUS_READY                (0 << 0) /*   0 = Not Busy                   */
#define STATUS_BUSY                 (1 << 0) /*   1 = Busy                       */
#define STATUS_WEL_MASK             (1 << 1) /* Bit 1: Write enable latch status */
#define STATUS_WEL_DISABLED         (0 << 1) /*   0 = Not Write Enabled          */
#define STATUS_WEL_ENABLED          (1 << 1) /*   1 = Write Enabled              */
#define STATUS_BP_SHIFT             (2)      /* Bits 2-4: Block protect bits     */
#define STATUS_BP_MASK              (7 << STATUS_BP_SHIFT)
#define STATUS_BP_NONE              (0 << STATUS_BP_SHIFT)
#define STATUS_BP_ALL               (15 << STATUS_BP_SHIFT)
#define STATUS_TB_MASK              (1 << 5) /* Bit 5: Top / Bottom Protect      */
#define STATUS_TB_TOP               (0 << 5) /*   0 = BP2-BP0 protect Top down   */
#define STATUS_TB_BOTTOM            (1 << 5) /*   1 = BP2-BP0 protect Bottom up  */
#define STATUS_SEC_MASK             (1 << 6) /* Bit 6: SEC                       */
#define STATUS_SEC_64KB             (0 << 6) /*   0 = Protect 64KB Blocks        */
#define STATUS_SEC_4KB              (1 << 6) /*   1 = Protect 4KB Sectors        */
#define STATUS_SRP_MASK             (1 << 7) /* Bit 7: Status register protect 0 */
#define STATUS_SRP_UNLOCKED         (0 << 7) /*   see blow for details           */
#define STATUS_SRP_LOCKED           (1 << 7) /*   see blow for details           */

/* Status register 2 bit definitions                                      */

#define STATUS2_QE_MASK             (1 << 1) /* Bit 1: Quad Enable (QE)          */
#define STATUS2_QE_DISABLED         (0 << 1) /*  0 = Standard/Dual SPI modes     */
#define STATUS2_QE_ENABLED          (1 << 1) /*  1 = Standard/Dual/Quad modes    */

/* W25Q032 (4 MB) memory capacity */

#define W25Q32_SECTOR_SIZE          (4 * 1024)
#define W25Q32_SECTOR_ERASE_TIME    (120)
#define W25Q32_SECTOR_SHIFT         (12)
#define W25Q32_SECTOR_COUNT         (1024)
#define W25Q32_PAGE_SIZE            (256)
#define W25Q32_PAGE_SHIFT           (8)


#define QSPI_FIFO_LEN_MAX           256

static void bk_qspi_flash_w25q_write_enable(qspi_id_t id);
static void bk_qspi_flash_w25q_wait_oip_done(qspi_id_t id);
static bk_err_t bk_qspi_flash_w25q_quad_enable(qspi_id_t id);

static bk_err_t bk_qspi_flash_w25q_init(void)
{
    // 11 ~ 12 MHz
    qspi_config_t config = {0};
    config.src_clk = QSPI_SCLK_320M;
    config.src_clk_div = 0x6;
    config.clk_div = 0x4;
    BK_LOG_ON_ERR(bk_qspi_init(1, &config));

#if QSPI_QUAD_ENABLE
    bk_qspi_flash_w25q_quad_enable(QSPI_ID_1);
#endif // QSPI_QUAD_ENABLE

    return BK_OK;
}

static bk_err_t bk_qspi_flash_w25q_deinit(void)
{
    BK_LOG_ON_ERR(bk_qspi_deinit(QSPI_ID_1));
    return BK_OK;
}

static uint32_t bk_qspi_flash_w25q_read_status1(qspi_id_t id)
{
    qspi_cmd_t read_status_cmd = {0};
    uint32_t status_reg_data = 0;

    read_status_cmd.device = QSPI_FLASH;
    read_status_cmd.wire_mode = QSPI_1WIRE;
    read_status_cmd.work_mode = INDIRECT_MODE;
    read_status_cmd.op = QSPI_READ;
    read_status_cmd.cmd = W25Q_READ_STATUS_1;
    read_status_cmd.data_len = 1;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
    bk_qspi_read(id, &status_reg_data, 1);

    return status_reg_data & 0xff;
}

static bk_err_t bk_qspi_flash_w25q_write_status1(qspi_id_t id, uint8_t status_reg_data)
{
    qspi_cmd_t write_status_cmd = {0};

    // 1, write enable
    bk_qspi_flash_w25q_write_enable(id);

    write_status_cmd.device = QSPI_FLASH;
    write_status_cmd.wire_mode = QSPI_1WIRE;
    write_status_cmd.work_mode = INDIRECT_MODE;
    write_status_cmd.op = QSPI_WRITE;
    write_status_cmd.cmd = (status_reg_data << 8) | W25Q_WRITE_STATUS_1;

    BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
    bk_qspi_flash_w25q_wait_oip_done(id);

    return BK_OK;
}

static uint32_t bk_qspi_flash_w25q_read_status2(qspi_id_t id)
{
    qspi_cmd_t read_status_cmd = {0};
    uint32_t status_reg_data = 0;

    read_status_cmd.device = QSPI_FLASH;
    read_status_cmd.wire_mode = QSPI_1WIRE;
    read_status_cmd.work_mode = INDIRECT_MODE;
    read_status_cmd.op = QSPI_READ;
    read_status_cmd.cmd = W25Q_READ_STATUS_2;
    read_status_cmd.data_len = 1;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
    bk_qspi_read(id, &status_reg_data, 1);

    return status_reg_data;
}

static bk_err_t bk_qspi_flash_w25q_write_status2(qspi_id_t id, uint8_t status_reg_data)
{
    qspi_cmd_t write_status_cmd = {0};

    bk_qspi_flash_w25q_write_enable(id);

    write_status_cmd.device = QSPI_FLASH;
    write_status_cmd.wire_mode = QSPI_1WIRE;
    write_status_cmd.work_mode = INDIRECT_MODE;
    write_status_cmd.op = QSPI_WRITE;
    write_status_cmd.cmd = (status_reg_data << 8) | W25Q_WRITE_STATUS_2;
    write_status_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
    bk_qspi_flash_w25q_wait_oip_done(id);

    return BK_OK;
}

static void bk_qspi_flash_w25q_wait_oip_done(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    for(int i = 0; i <= 2000; i++) {
        status_reg_data = bk_qspi_flash_w25q_read_status1(id);
        if(STATUS_READY == (status_reg_data & STATUS_BUSY_MASK)) {
            break;
        }
        if(i == 2000) {
            QSPI_LOGW("[%s]: wait flsh progress done timeout.\n", __func__);
        }
        rtos_delay_milliseconds(1);
    }
}

static void bk_qspi_flash_w25q_write_enable(qspi_id_t id)
{
    qspi_cmd_t wren_cmd = {0};

    wren_cmd.device = QSPI_FLASH;
    wren_cmd.wire_mode = QSPI_1WIRE;
    wren_cmd.work_mode = INDIRECT_MODE;
    wren_cmd.op = QSPI_WRITE;
    wren_cmd.cmd = W25Q_WRITE_ENABLE;
    wren_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &wren_cmd));
    bk_qspi_flash_w25q_wait_oip_done(id);
}

static bk_err_t bk_qspi_flash_w25q_quad_enable(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)bk_qspi_flash_w25q_read_status2(id);
    if ((status_reg_data & STATUS2_QE_MASK) == STATUS2_QE_DISABLED) {
        status_reg_data |= STATUS2_QE_ENABLED;
        return bk_qspi_flash_w25q_write_status2(id, status_reg_data);
    }
    return BK_OK;
}

static bk_err_t bk_qspi_flash_w25q_quad_disable(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)bk_qspi_flash_w25q_read_status2(id);
    if ((status_reg_data & STATUS2_QE_MASK) != STATUS2_QE_DISABLED) {
        status_reg_data &= ~STATUS2_QE_ENABLED;
        return bk_qspi_flash_w25q_write_status2(id, status_reg_data);
    }
    return BK_OK;
}

static bk_err_t bk_qspi_flash_w25q_set_protect_none(void)
{
    uint8_t status_reg_data = 0;

    status_reg_data = bk_qspi_flash_w25q_read_status1(QSPI_ID_1) & 0xff;
    uint8_t clean_bits = ~STATUS_BP_MASK;
    status_reg_data &= clean_bits;
    bk_qspi_flash_w25q_write_status1(QSPI_ID_1, status_reg_data);
    return BK_OK;
}

static uint32_t bk_qspi_flash_w25q_read_id(void)
{
    qspi_cmd_t read_id_cmd = {0};
    uint32_t read_id_data = 0;

    read_id_cmd.device = QSPI_FLASH;
    read_id_cmd.wire_mode = QSPI_1WIRE;
    read_id_cmd.work_mode = INDIRECT_MODE;
    read_id_cmd.op = QSPI_READ;
    read_id_cmd.cmd = W25Q_JEDEC_ID;
    read_id_cmd.data_len = 4;

    BK_LOG_ON_ERR(bk_qspi_command(QSPI_ID_1, &read_id_cmd));

    bk_qspi_read(QSPI_ID_1, &read_id_data, 4);

    return read_id_data;
}

static bk_err_t bk_qspi_flash_w25q_erase_sector(uint32_t addr)
{
    if (addr > (W25Q32_SECTOR_SIZE * W25Q32_SECTOR_COUNT))
        return -1;

    qspi_cmd_t erase_sector_cmd = {0};
    uint32_t sa = addr & ~(W25Q32_SECTOR_SIZE - 1);

    // bk_printf("erase sector: %x %x\r\n", addr, sa);

    bk_qspi_flash_w25q_write_enable(QSPI_ID_1);

    erase_sector_cmd.device = QSPI_FLASH;
    erase_sector_cmd.wire_mode = QSPI_1WIRE;
    erase_sector_cmd.work_mode = INDIRECT_MODE;
    erase_sector_cmd.op = QSPI_WRITE;
    erase_sector_cmd.cmd = W25Q_SECTOR_ERASE;
    erase_sector_cmd.addr = sa;
    erase_sector_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;
    erase_sector_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(QSPI_ID_1, &erase_sector_cmd));
    bk_qspi_flash_w25q_wait_oip_done(QSPI_ID_1);

    return BK_OK;
}

static bk_err_t bk_qspi_flash_w25q_standrad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
    // bk_printf("===> entry %s\r\n", __func__);
    if ((data == NULL) || (size == 0) || (size > W25Q32_PAGE_SIZE))
        return -1;

    qspi_cmd_t page_program_cmd = {0};
    uint32_t cmd_data_len = 0;

    // 1, write enable
    bk_qspi_flash_w25q_write_enable(QSPI_ID_1);

    // 2, store data into qspi fifo
    cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
    bk_qspi_write(id, data, cmd_data_len);

    // 3, send data to flash
    page_program_cmd.device = QSPI_FLASH;
    page_program_cmd.wire_mode = QSPI_1WIRE;
    page_program_cmd.work_mode = INDIRECT_MODE;
    page_program_cmd.op = QSPI_WRITE;
    page_program_cmd.cmd = W25Q_PAGE_PROGRAM;

    page_program_cmd.addr = addr;
    page_program_cmd.data_len = cmd_data_len;
    BK_LOG_ON_ERR(bk_qspi_command(id, &page_program_cmd));

    bk_qspi_flash_w25q_wait_oip_done(QSPI_ID_1);

    // bk_printf("===< exit %s\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_w25q_standrad_page_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
    // bk_printf("===> entry %s\r\n", __func__);
    if ((data == NULL) || (size == 0) || (size > W25Q32_PAGE_SIZE))
        return BK_FAIL;

    qspi_cmd_t single_read_cmd = {0};

    single_read_cmd.device = QSPI_FLASH;
    single_read_cmd.wire_mode = QSPI_1WIRE;
    single_read_cmd.work_mode = INDIRECT_MODE;
    single_read_cmd.op = QSPI_READ;
    single_read_cmd.cmd = W25Q_PAGE_READ;

    single_read_cmd.addr = addr;
    single_read_cmd.data_len = size;
    BK_LOG_ON_ERR(bk_qspi_command(id, &single_read_cmd));
    bk_qspi_read(id, data, size);

    // bk_printf("<=== exit %s\r\n\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_w25q_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
    // bk_printf("===> entry %s\r\n", __func__);
    if ((data == NULL) || (size == 0) || (size > W25Q32_PAGE_SIZE))
        return -1;

    bk_qspi_flash_w25q_write_enable(id);

    qspi_cmd_t page_program_cmd = {0};
    uint32_t cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;

    page_program_cmd.device = QSPI_FLASH;
    page_program_cmd.wire_mode = QSPI_4WIRE;
    page_program_cmd.work_mode = INDIRECT_MODE;
    page_program_cmd.op = QSPI_WRITE;
    page_program_cmd.cmd = W25Q_QUAD_PAGE_PROGRAM;

    bk_qspi_write(id, data, cmd_data_len);
    page_program_cmd.addr = addr;
    page_program_cmd.data_len = cmd_data_len;
    BK_LOG_ON_ERR(bk_qspi_command(id, &page_program_cmd));

    bk_qspi_flash_w25q_wait_oip_done(id);

    // bk_printf("<=== exit %s\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_w25q_quad_page_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
    // bk_printf("===> entry %s\r\n", __func__);
    if ((data == NULL) || (size != W25Q32_PAGE_SIZE))
        return BK_FAIL;

    qspi_cmd_t quad_read_cmd = {0};

    quad_read_cmd.device = QSPI_FLASH;
    quad_read_cmd.wire_mode = QSPI_4WIRE;
    quad_read_cmd.work_mode = INDIRECT_MODE;
    quad_read_cmd.op = QSPI_READ;
    quad_read_cmd.cmd = W25Q_QUAD_PAGE_READ;
    quad_read_cmd.dummy_cycle = 8;
    quad_read_cmd.addr = addr;
    quad_read_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;
    quad_read_cmd.data_len = size;

    BK_LOG_ON_ERR(bk_qspi_command(id, &quad_read_cmd));
    bk_qspi_read(id, data, size);

    // bk_printf("<=== exit %s\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_w25q_page_read(uint32_t addr, void *data, uint32_t size)
{

#if (QSPI_QUAD_ENABLE == 1)
    return bk_qspi_flash_w25q_quad_page_read(1, addr, data, size);
#else // QSPI_QUAD_ENABLE == 0
    return bk_qspi_flash_w25q_standrad_page_read(1, addr, data, size);
#endif

}

static bk_err_t bk_qspi_flash_w25q_page_program(uint32_t addr, const void *data, uint32_t size)
{
#if (QSPI_QUAD_ENABLE == 1)
    return bk_qspi_flash_w25q_quad_page_program(1, addr, data, size);
#else // QSPI_QUAD_ENABLE == 0
    return bk_qspi_flash_w25q_standrad_page_program(1, addr, data, size);
#endif
}

qspi_driver_desc_t qspi_w25q_desc = {
    .name = "w25q32",
    .page_size = W25Q32_PAGE_SIZE,
    .block_size = W25Q32_SECTOR_SIZE,
    .total_size = W25Q32_SECTOR_SIZE * W25Q32_SECTOR_COUNT,

    .init = bk_qspi_flash_w25q_init,
    .deinit = bk_qspi_flash_w25q_deinit,
    .read_id = bk_qspi_flash_w25q_read_id,
    .unblock = bk_qspi_flash_w25q_set_protect_none,
    .read_page = bk_qspi_flash_w25q_page_read,
    .write_page = bk_qspi_flash_w25q_page_program,
    .erase_block = bk_qspi_flash_w25q_erase_sector,
};


