#include "sflash.h"
#include <os/mem.h>

#if CONFIG_SPI_MST_FLASH
#define TAG "sflash"
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define SFLASH_MAX_CAPACITY (16*1024*1024)  // unit: byte.
#define SFLASH_SECTOR_SIZE  (4*1024)        // unit: byte.
#define SFLASH_PAGE_SIZE    (256)           // unit: byte.

#define SFLASH_CHECK_WORD_ALIGN (0)         // check word align.
#define SFLASH_WORD_SIZE    (4)             // unit: byte.

#define SFLASH_SPI_BAUD_RATE	10000000//26000000    // baudrate


extern int spi_flash_read(uint32_t addr, uint32_t size, uint8_t *dst);
extern int spi_flash_write(uint32_t addr, uint32_t size, uint8_t *src);
extern int spi_flash_erase(uint32_t addr, uint32_t size);


/**
  * @brief  qflash printf buff.
  * @param  buff[in] pointer to buff.
  *         len[in] buff length.
  *         space[in] whether or not insert space.
  * 		num_per_line[in] number per line.
  *         end_new_line[in] whether or not change to new line in end.
  * @retval none.
  */
void sflash_printf_buff(const uint8_t *buff, uint32_t len, uint32_t num_per_line, bool space, bool end_new_line)
{
    if ( (buff == NULL) || (len == 0) )
    {
        return;
    }

#if 0
    uint8_t temp_printf_sync = bk_get_printf_sync()? 1 : 0;
    bk_set_printf_sync(true);

    if (num_per_line == 0)
    {
        num_per_line = 64;
    }

    for (uint32_t temp=1; temp<=len; temp++)
    {
        if ((temp % num_per_line) == 0)
        {
            BK_DUMP_OUT("%02X", buff[temp-1]);
            BK_DUMP_OUT("\r\n");
        }
        else
        {
            if (space == true)
            {
                BK_DUMP_OUT("%02X ", buff[temp-1]);
            }
            else
            {
                BK_DUMP_OUT("%02X", buff[temp-1]);
            }
        }
    }

    if (end_new_line)
    {
        // BK_DUMP_OUT("\r\n");
    }

    bk_set_printf_sync(temp_printf_sync);
#else
    unsigned char buffer[64] = {0};
    int i = 0;

    os_memset(buffer, 0, sizeof(buffer));
    for(i=0; i<len; i++)
    {
        if((i+1)%16 == 0)
        {
            os_snprintf(buffer+os_strlen(buffer), sizeof(buffer), "%02x\r\n", buff[i]);
            bk_printf("%s", buffer);
            os_memset(buffer, 0, sizeof(buffer));
        }
        else
        {
            os_snprintf(buffer+os_strlen(buffer), sizeof(buffer), "%02x ", buff[i]);
        }
    }
    if(os_strlen(buffer))
        bk_printf("%s\r\n", buffer);
#endif

    return;
}

bk_err_t sflash_init(void)
{
    bk_err_t ret = BK_OK;

    ret = bk_spi_driver_init();
    if (BK_OK != ret)
    {
        LOGE("[%s] bk_spi_driver_init fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }

    spi_config_t config = {0};
	config.role = SPI_ROLE_MASTER;
	config.bit_width = SPI_BIT_WIDTH_8BITS;
	config.polarity = 0;
	config.phase = 0;
	config.wire_mode = SPI_4WIRE_MODE;
	config.baud_rate = SFLASH_SPI_BAUD_RATE;
	config.bit_order = SPI_MSB_FIRST;
#if CONFIG_SPI_DMA
	config.dma_mode = 1;
	config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_DTCM);
	config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_DTCM);
#endif
    ret = bk_spi_init(SPI_FLASH_ID, &config);
    if (BK_OK != ret)
    {
        LOGE("[%s] bk_spi_init fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }

    return ret;
}

bk_err_t sflash_deinit(void)
{
    bk_err_t ret = BK_OK;

    ret = bk_spi_deinit(SPI_FLASH_ID);
    if (BK_OK != ret)
    {
        LOGE("[%s] bk_spi_deinit fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }

    ret = bk_spi_driver_deinit();
    if (BK_OK != ret)
    {
        LOGE("[%s] bk_spi_driver_deinit fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }

    return ret;
}

bk_err_t sflash_erase(uint32_t addr, uint32_t size)
{
    LOGD("[%s] addr=0x%08X, size=%u.\r\n", __func__, addr, size);

    if ( (addr >= SFLASH_MAX_CAPACITY) || (size > SFLASH_MAX_CAPACITY) || ((addr + size) > SFLASH_MAX_CAPACITY) )
    {
        LOGE("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    #if (SFLASH_CHECK_WORD_ALIGN)
        if ( (addr%SFLASH_WORD_SIZE) || (size%SFLASH_WORD_SIZE) )
        {
            LOGE("[%s] addr or size paras word align error!\r\n", __func__);
            return BK_FAIL;
        }
    #endif

    if ( 0 == size )
    {
        LOGE("[%s] buff or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_OK;


    #if (CONFIG_TASK_WDT)
        extern void bk_task_wdt_feed(void);
        bk_task_wdt_feed();
    #endif

    ret = spi_flash_erase(addr, size);
    if (BK_OK != ret)
    {
        LOGE("[%s] spi_flash_erase fail!\r\n", __func__);
        return BK_FAIL;
    }

    return ret;
}

bk_err_t sflash_read(uint32_t addr, uint8_t *buff, uint32_t size)
{
    LOGD("[%s] addr=0x%08X, buff=0x%08X, size=%u.\r\n", __func__, addr, buff, size);

    bk_err_t ret = BK_OK;

    if ( (addr >= SFLASH_MAX_CAPACITY) || (size > SFLASH_MAX_CAPACITY) || ((addr + size) > SFLASH_MAX_CAPACITY) )
    {
        LOGE("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ( (NULL==buff) || (0 == size) )
    {
        LOGE("[%s] buff or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    #if (SFLASH_CHECK_WORD_ALIGN)
        if ( (addr%SFLASH_WORD_SIZE) || (size%SFLASH_WORD_SIZE) )
        {
            LOGE("[%s] addr or size paras word align error!\r\n", __func__);
            return BK_FAIL;
        }
    #endif

    #if (CONFIG_TASK_WDT)
        extern void bk_task_wdt_feed(void);
        bk_task_wdt_feed();
    #endif

    ret = spi_flash_read(addr, size, buff);
    if (BK_OK != ret)
    {
        LOGE("[%s] spi_flash_read fail[ret=%d]!\r\n", __func__, ret);
        return BK_FAIL;
    }

    return ret;
}

bk_err_t sflash_write(uint32_t addr, const uint8_t *buff, uint32_t size)
{
    LOGD("[%s] addr=0x%08X, buff=0x%08X, size=%u.\r\n", __func__, addr, buff, size);

    bk_err_t ret = BK_OK;

    if ( (addr >= SFLASH_MAX_CAPACITY) || (size > SFLASH_MAX_CAPACITY) || ((addr + size) > SFLASH_MAX_CAPACITY) )
    {
        LOGE("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ( (NULL==buff) || (0 == size) )
    {
        LOGE("[%s] buff or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    #if (SFLASH_CHECK_WORD_ALIGN)
        if ( (addr%SFLASH_WORD_SIZE) || (size%SFLASH_WORD_SIZE) )
        {
            LOGE("[%s] addr or size paras word align error!\r\n", __func__);
            return BK_FAIL;
        }
    #endif

    uint32_t head_addr = 0, head_size = 0;
    uint32_t mid_addr = 0, mid_size = 0;
    uint32_t tail_addr = 0, tail_size = 0;

    if (addr%SFLASH_PAGE_SIZE)
    {
        head_addr = addr;
        head_size = ( size > (SFLASH_PAGE_SIZE - (addr%SFLASH_PAGE_SIZE)) )?  (SFLASH_PAGE_SIZE - (addr%SFLASH_PAGE_SIZE)) : size;
        size -= head_size;
    }

    if (size >= SFLASH_PAGE_SIZE)
    {
        mid_addr = addr + head_size;
        mid_size = (size / SFLASH_PAGE_SIZE) * SFLASH_PAGE_SIZE;
        size -= mid_size;
    }

    if (size > 0)
    {
        tail_addr = addr + head_size + mid_size;
        tail_size = size;
    }

    LOGD("[%s] h_addr=0x%08X, h_size=%u, m_addr=0x%08X, m_size=%u, t_addr=0x%08X, t_size=%u.\r\n", __func__, head_addr, head_size, mid_addr, mid_size, tail_addr, tail_size);

    if (head_size)
    {
        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        ret = spi_flash_write(head_addr, head_size, (uint8_t *)buff);
        if (BK_OK != ret)
        {
            LOGE("[%s] spi_flash_write head fail[ret=%d]!\r\n", __func__, ret);
            return ret;
        }
    }

    if (mid_size)
    {
        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        ret = spi_flash_write(mid_addr, mid_size, (uint8_t *)(buff+head_size));
        if (BK_OK != ret)
        {
            LOGE("[%s] spi_flash_write middle fail[ret=%d]!\r\n", __func__, ret);
            return ret;
        }
    }

    if (tail_size)
    {
        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        ret = spi_flash_write(tail_addr, tail_size, (uint8_t *)(buff+head_size+mid_size));
        if (BK_OK != ret)
        {
            LOGE("[%s] spi_flash_write tail fail[ret=%d]!\r\n", __func__, ret);
            return ret;
        }
    }

    return ret;
}
#else
bk_err_t sflash_init(void)
{
    bk_printf("[%s] CONFIG_SPI_MST_FLASH don't open, so external spi flash don't support\r\n", __FUNCTION__);
    return -1;
}
#endif
