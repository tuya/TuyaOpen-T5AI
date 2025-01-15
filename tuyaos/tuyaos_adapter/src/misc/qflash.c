#include <string.h>
#include "qflash.h"
#include <os/mem.h>
#include "sdkconfig.h"
#include <driver/qspi_flash_common.h>
#include "tkl_memory.h"

#if CONFIG_QSPI

// default
static uint32_t __g_qspi_flash_capacity = 128 * 1024 * 1024; // 128MB
static uint32_t __g_qspi_flash_page_size = 2048;
static uint32_t __g_qspi_flash_block_size = 131072;     // 128K

#define QFLASH_DEBUG_ON     0

static qspi_driver_desc_t *qflash_dev = NULL;

/***************************************************************************************
 *  for debug
 * */
static inline uint32_t __get_start_block(uint32_t address)
{
    return (address / __g_qspi_flash_block_size);
}

static inline uint32_t __get_block_num(uint32_t addr, uint32_t size)
{
    uint32_t curr_block_addr = (addr / __g_qspi_flash_block_size) * __g_qspi_flash_block_size;
    uint32_t next_block_addr = curr_block_addr + __g_qspi_flash_block_size;

    if (addr + size <= next_block_addr)
        return 1;
    else {
        uint32_t first_block_occupied = next_block_addr - addr;
        return (size - first_block_occupied) / __g_qspi_flash_block_size + 2;
    }
}

static inline uint32_t __get_start_page(uint32_t address)
{
    uint32_t first_block_offset = address & (__g_qspi_flash_block_size - 1);
    return first_block_offset / __g_qspi_flash_page_size;
}

static inline uint32_t __get_page_num(uint32_t addr, uint32_t size)
{
    uint32_t curr_page_addr = (addr / __g_qspi_flash_page_size) * __g_qspi_flash_page_size;
    uint32_t next_page_addr = curr_page_addr + __g_qspi_flash_page_size;

    if (addr + size <= next_page_addr)
        return 1;
    else {
        uint32_t first_page_occupied = next_page_addr - addr;
        if (((size - first_page_occupied) % __g_qspi_flash_page_size) == 0)
            return (size - first_page_occupied) / __g_qspi_flash_page_size + 1;
        else
            return (size - first_page_occupied) / __g_qspi_flash_page_size + 2;
    }
}




static inline bool __is_power_of_two(uint32_t n)
{
    if ((n & (n - 1)) == 0)
        return 1;
    else
        return 0;
}

bk_err_t qflash_init(void)
{
    bk_err_t ret = BK_OK;

    ret = bk_qspi_driver_init();
    if (BK_OK != ret) {
        bk_printf("[%s] bk_qspi_driver_init fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }

    qflash_dev = tuya_qspi_device_query(CONFIG_TUYA_QSPI_FLASH_TYPE);
    if (qflash_dev == NULL) {
        bk_printf("Not found qspi flash %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
        return BK_FAIL;
    }

    ret = qflash_dev->init();
    if (BK_OK != ret) {
        bk_printf("[%s] bk_qspi_init fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }

    ret = qflash_dev->unblock();


    __g_qspi_flash_capacity = qflash_dev->total_size;
    __g_qspi_flash_page_size = qflash_dev->page_size;
    __g_qspi_flash_block_size = qflash_dev->block_size;

    if (!__is_power_of_two(__g_qspi_flash_capacity) || \
        !__is_power_of_two(__g_qspi_flash_page_size) || \
        !__is_power_of_two(__g_qspi_flash_block_size)) {
        bk_printf("flash parameter error, capacity/page/block size must be power of 2\r\n");
        return -1;
    }

    bk_printf("[%s] qspi flash, capacity: %dMB, block: %dKB, page: %d\r\n",
            __func__,
            __g_qspi_flash_capacity >> 20,
            __g_qspi_flash_block_size >> 10,
            __g_qspi_flash_page_size);
    return ret;
}

bk_err_t qflash_deinit(void)
{
    bk_err_t ret = BK_OK;

    if (qflash_dev == NULL) {
        bk_printf("[%s] No qspi flash\r\n", __func__);
        return BK_FAIL;
    }

    ret = qflash_dev->deinit();
    if (BK_OK != ret) {
        bk_printf("[%s] bk_qspi_deinit fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }

    ret = bk_qspi_driver_deinit();
    if (BK_OK != ret) {
        bk_printf("[%s] bk_qspi_driver_deinit fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }

    return ret;
}

bk_err_t qflash_erase(uint32_t addr, uint32_t size)
{
#if QFLASH_DEBUG_ON
    bk_printf("[%s]addr=0x%08X, size=0x%08x, block=%04x, num=%d\r\n",
            __func__, addr, size, __get_start_block(addr), __get_block_num(addr, size));
#endif

    if (qflash_dev == NULL) {
        bk_printf("[%s] No qspi flash\r\n", __func__);
        return BK_FAIL;
    }

    if ((addr >= __g_qspi_flash_capacity) || \
        (size > __g_qspi_flash_capacity) || \
        ((addr + size) > __g_qspi_flash_capacity)) {
        bk_printf("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if (0 == size) {
        bk_printf("[%s] buf or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_OK;
    uint32_t temp;

    for (temp = 0; temp < size; temp += __g_qspi_flash_block_size) {
        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        ret = qflash_dev->erase_block(addr + temp);
        if (BK_OK != ret) {
            bk_printf("[%s] bk_qspi_flash_erase_sector fail!\r\n", __func__);
            return BK_FAIL;
        }
    }

#if QFLASH_DEBUG_ON
    bk_printf("[%s] erase block range[%04x ~ %04x].\r\n", __func__,
            addr/__g_qspi_flash_block_size,
            addr/__g_qspi_flash_block_size + temp/__g_qspi_flash_block_size - 1);
#endif

    return ret;
}

bk_err_t qflash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    bk_err_t ret = BK_OK;

    if (qflash_dev == NULL) {
        bk_printf("[%s] No qspi flash\r\n", __func__);
        return BK_FAIL;
    }

    if ((addr >= __g_qspi_flash_capacity) || \
        (size > __g_qspi_flash_capacity) || \
        ((addr + size) > __g_qspi_flash_capacity)) {
        bk_printf("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ((NULL == data) || (0 == size)) {
        bk_printf("[%s] data or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    uint8_t *buf = (uint8_t *)tkl_system_malloc(__g_qspi_flash_page_size);
    if (buf == NULL) {
        bk_printf("read flash error, malloc failed\r\n");
        return -1;
    }
    os_memset(buf, 0, __g_qspi_flash_page_size);

    memset(data, 0, size);

#if (QFLASH_DEBUG_ON == 1)
    bk_printf("[%s] addr=0x%08X, size=%u, start block/page=%04x/%02x, num of block/page=%d/%d\r\n",
            __func__, addr, size, __get_start_block(addr), __get_start_page(addr),
            __get_block_num(addr, size), __get_page_num(addr, size));
#endif

    /*
        for example: read 0x23456, len 5100

        0x0 ...    0x23000          0x23800             0x24000             0x24800             0x25000
        |           |                   |                   |                   |                   |
        |-----------|-------------------|-------------------|-------------------|-------------------|------------
                                 |938   |      2048         |      2048         |66|
                                 |      |                   |                   |  |                |
                              0x23456   |                                       |  0x24842

        start_page_address:     0x23456 & ~page_mask = 0x23000
        start_page_offset:      0x23456 & page_mask = 0x456
        start_page_left_len:    2048 - 0x456 = 938

        continue_page_num = 2
        continue_page_start_addr: start_page_address + 2048 = 0x23800

        last_page_address: continue_page_start_addr + n * 2048 = 0x24800
        last_page_offset:  0
        last_page_len:  total_len - start_paeg_len - n * 2048 = 5100 - 938 - 4096 = 66
    */

    uint32_t page_mask = __g_qspi_flash_page_size - 1;
    // 1. 1st page handle
    uint32_t first_page_addr = addr & ~page_mask;
    uint32_t first_page_offset = addr & page_mask;
    uint32_t first_page_left_len = __g_qspi_flash_page_size - first_page_offset;
    uint32_t first_page_copy_len = 0;

    if (size < first_page_left_len) {
        first_page_copy_len = size;
    } else {
        first_page_copy_len = first_page_left_len;
    }

#if (QFLASH_DEBUG_ON == 1)
    bk_printf("read: 1st, addr:%08x, len:%04x, buffer ofs:%x\r\n", first_page_addr, first_page_copy_len, 0);
#endif

    qflash_dev->read_page(first_page_addr, buf, __g_qspi_flash_page_size);
    os_memcpy(data, buf + first_page_offset, first_page_copy_len);

    if (size == first_page_copy_len) {
        tkl_system_free(buf);
        buf = NULL;
        return 0;
    }

    // 2. whole pages continue
    uint32_t cuntinue_page_num = (size - first_page_copy_len) / __g_qspi_flash_page_size;
    uint32_t cuntinue_start_addr = first_page_addr + __g_qspi_flash_page_size;
    for (int i = 0; i < cuntinue_page_num; i++) {
        uint32_t tmp_addr = cuntinue_start_addr + i * __g_qspi_flash_page_size;
        uint32_t r_ofs = first_page_copy_len + i * __g_qspi_flash_page_size;
        qflash_dev->read_page(tmp_addr, (uint8_t *)data + r_ofs, __g_qspi_flash_page_size);
#if (QFLASH_DEBUG_ON == 1)
        bk_printf("read: 2nd, addr:%08x, len:%04x, buffer ofs:%x\r\n", tmp_addr, __g_qspi_flash_page_size, r_ofs);
#endif
    }

    if (size <= (first_page_copy_len + cuntinue_page_num * __g_qspi_flash_page_size)) {
        tkl_system_free(buf);
        buf = NULL;
        return 0;
    }

    // 3. last page handle
    uint32_t last_page_start_addr = cuntinue_start_addr + cuntinue_page_num * __g_qspi_flash_page_size;
    uint32_t last_ofs = first_page_copy_len + cuntinue_page_num * __g_qspi_flash_page_size;
    uint32_t last_copy_len = size - last_ofs;

#if (QFLASH_DEBUG_ON == 1)
    bk_printf("read: 3rd, addr:%08x, buffer ofs:%x\r\n", last_page_start_addr, last_ofs);
#endif
    qflash_dev->read_page(last_page_start_addr, buf, __g_qspi_flash_page_size);
    os_memcpy((uint8_t *)data + last_ofs, buf, last_copy_len);

    // DEBUG
#if (QFLASH_DEBUG_ON > 1)
    bk_printf("------------------------------>\r\n");
    bk_printf("[%s] addr=0x%08X, size=%u\r\n", __func__, addr, size);
    for (int i = 0; i < size; i++) {
        if ((i & 0x1f) == 0 && (i != 0))
            bk_printf("\r\n");
        bk_printf("%02x ", buf[i]);
    }
    bk_printf("\r\n<------------------------------\r\n\r\n");
#endif

    tkl_system_free(buf);
    buf = NULL;
    return ret;
}

bk_err_t qflash_write(const uint32_t addr, const uint8_t *data, const uint32_t size)
{
    bk_err_t ret = BK_OK;

    if (qflash_dev == NULL) {
        bk_printf("[%s] No qspi flash\r\n", __func__);
        return BK_FAIL;
    }

    if ((addr >= __g_qspi_flash_capacity) || \
        (size > __g_qspi_flash_capacity) || \
        ((addr + size) > __g_qspi_flash_capacity)) {
        bk_printf("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ((NULL == data) || (0 == size)) {
        bk_printf("[%s] data or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

#if (QFLASH_DEBUG_ON == 1)
    bk_printf("[%s] addr=0x%08X, size=%u, start block/page=%04x/%02x, num of block/page=%d/%d\r\n",
            __func__, addr, size, __get_start_block(addr), __get_start_page(addr),
            __get_block_num(addr, size), __get_page_num(addr, size));
#endif

    /*
        for example: write 0x23456, len 5100

        0x0 ...    0x23000          0x23800             0x24000             0x24800             0x25000
        |           |                   |                   |                   |                   |
        |-----------|-------------------|-------------------|-------------------|-------------------|------------
                                 |938   |      2048         |      2048         |66|
                                 |      |                   |                   |  |                |
                              0x23456   |                                       |  0x24842

        start_page_address: 0x23456 & ~page_mask = 0x23000
        start_page_offset:  0x23456 & page_mask = 0x456
        start_paeg_len:     2048 - 0x456 = 938

        continue_page_num = 2
        continue_page_start_addr: start_page_address + 2048 = 0x23800

        last_page_address: continue_page_start_addr + n * 2048 = 0x24800
        last_page_offset:  0
        last_page_len:  total_len - start_paeg_len - n * 2048 = 5100 - 938 - 4096 = 66
    */

    uint8_t *buf = (uint8_t *)tkl_system_malloc(__g_qspi_flash_page_size);
    if (buf == NULL) {
        bk_printf("read flash error, malloc failed\r\n");
        return -1;
    }
    os_memset(buf, 0, __g_qspi_flash_page_size);

    uint32_t page_mask = __g_qspi_flash_page_size - 1;
    // 1. 1st page handle
    uint32_t first_page_addr = addr & ~page_mask;
    uint32_t first_page_offset = addr & page_mask;
    uint32_t first_page_left_len = __g_qspi_flash_page_size - first_page_offset;
    uint32_t first_page_write_len = 0;

    if (size < first_page_left_len) {
        first_page_write_len = size;
    } else {
        first_page_write_len = first_page_left_len;
    }

#if (QFLASH_DEBUG_ON == 1)
    bk_printf("write: 1st, addr:%08x, len:%04x, buffer ofs:%x\r\n", first_page_addr, first_page_write_len, first_page_offset);
#endif
    qflash_dev->read_page(first_page_addr, buf, __g_qspi_flash_page_size);
    os_memcpy(buf + first_page_offset, data, first_page_write_len);
    qflash_dev->write_page(first_page_addr, buf, __g_qspi_flash_page_size);

    if (size <= first_page_left_len) {
        tkl_system_free(buf);
        buf = NULL;
        return 0;
    }

    // 2. whole pages continue
    uint32_t cuntinue_page_num = (size - first_page_left_len) / __g_qspi_flash_page_size;
    uint32_t cuntinue_start_addr = first_page_addr + __g_qspi_flash_page_size;
    for (int i = 0; i < cuntinue_page_num; i++) {
        uint32_t tmp_addr = cuntinue_start_addr + i * __g_qspi_flash_page_size;
        uint32_t w_ofs = first_page_write_len + i * __g_qspi_flash_page_size;
        qflash_dev->read_page(tmp_addr, buf, __g_qspi_flash_page_size);
        os_memset(buf, 0, __g_qspi_flash_page_size);
        os_memcpy(buf, data + w_ofs,  __g_qspi_flash_page_size);
        qflash_dev->write_page(tmp_addr, buf, __g_qspi_flash_page_size);
#if (QFLASH_DEBUG_ON == 1)
        bk_printf("write: 2nd, addr:%08x, len:%04x, buffer ofs:%x\r\n", tmp_addr, __g_qspi_flash_page_size, w_ofs);
#endif
    }

    if (size <= (first_page_write_len + cuntinue_page_num * __g_qspi_flash_page_size)) {
        tkl_system_free(buf);
        buf = NULL;
        return 0;
    }

    // 3. last page handle
    uint32_t last_page_start_addr = cuntinue_start_addr + cuntinue_page_num * __g_qspi_flash_page_size;
    uint32_t last_page_write_len = (size - first_page_write_len) % __g_qspi_flash_page_size;
    uint32_t last_ofs = first_page_write_len + cuntinue_page_num * __g_qspi_flash_page_size;

#if (QFLASH_DEBUG_ON == 1)
    bk_printf("write: 3rd, addr:%08x, len:%04x, buffer ofs:%x\r\n", last_page_start_addr, last_page_write_len, last_ofs);
#endif

    os_memset(buf, 0, __g_qspi_flash_page_size);
    qflash_dev->read_page(last_page_start_addr, buf, __g_qspi_flash_page_size);
    os_memcpy(buf, (uint8_t *)data + last_ofs, last_page_write_len);
    qflash_dev->write_page(last_page_start_addr, buf, __g_qspi_flash_page_size);

    tkl_system_free(buf);
    buf = NULL;

#if (QFLASH_DEBUG_ON > 1)
    bk_printf("------------------------------>\r\n");
    bk_printf("[%s] addr=0x%08X, size=%u\r\n", __func__, addr, size);
    for (int i = 0; i < size; i++) {
        if ((i & 0x1f) == 0 && (i != 0))
            bk_printf("\r\n");
        bk_printf("%02x ", data[i]);
    }
    bk_printf("\r\n<------------------------------\r\n\r\n");
#endif

    return ret;
}
#else
bk_err_t qflash_init(void)
{
    bk_printf("[%s] CONFIG_QSPI don't open, so external qspi flash don't support\r\n", __FUNCTION__);
    return -1;
}
#endif
