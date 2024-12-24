#include "lfs_flashbd.h"

// Modified by TUYA Start
#include <os/os.h>
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
// Modified by TUYA End
#include <common/bk_include.h>
#include <driver/flash_types.h>
#include <driver/flash.h>
#include <driver/spi.h>
#include <driver/dma.h>

int lfs_flashbd_createcfg(const struct lfs_config *cfg,
        const struct lfs_flashbd_config *bdcfg) {
    //LFS_FLASHBD_TRACE("lfs_flashbd_createcfg(%p {.context=%p, "
    bk_printf("lfs_flashbd_createcfg(%p {.context=%p, "
                ".read=%p, .prog=%p, .erase=%p, .sync=%p, "
                ".read_size=%"PRIu32", .prog_size=%"PRIu32", "
                ".block_size=%"PRIu32", .block_count=%"PRIu32"}, "
                "%p)\r\n",
            (void*)cfg, cfg->context,
            (void*)(uintptr_t)cfg->read, (void*)(uintptr_t)cfg->prog,
            (void*)(uintptr_t)cfg->erase, (void*)(uintptr_t)cfg->sync,
            cfg->read_size, cfg->prog_size, cfg->block_size, cfg->block_count,
            (void*)bdcfg);
    lfs_flashbd_t *bd = cfg->context;
    bd->cfg = bdcfg;

    LFS_FLASHBD_TRACE("lfs_flashbd_createcfg -> %d", 0);
    return 0;
}

int lfs_flashbd_create(const struct lfs_config *cfg) {
    LFS_FLASHBD_TRACE("lfs_flashbd_create(%p {.context=%p, "
                ".read=%p, .prog=%p, .erase=%p, .sync=%p, "
                ".read_size=%"PRIu32", .prog_size=%"PRIu32", "
                ".block_size=%"PRIu32", .block_count=%"PRIu32"})",
            (void*)cfg, cfg->context,
            (void*)(uintptr_t)cfg->read, (void*)(uintptr_t)cfg->prog,
            (void*)(uintptr_t)cfg->erase, (void*)(uintptr_t)cfg->sync,
            cfg->read_size, cfg->prog_size, cfg->block_size, cfg->block_count);
    static const struct lfs_flashbd_config defaults = {.erase_value=-1};
    int err = lfs_flashbd_createcfg(cfg, &defaults);
    LFS_FLASHBD_TRACE("lfs_flashbd_create -> %d", err);
    return err;
}

int lfs_flashbd_destroy(const struct lfs_config *cfg) {
    LFS_FLASHBD_TRACE("lfs_flashbd_destroy(%p)", (void*)cfg);
    // clean up
    lfs_flashbd_t *bd = cfg->context;
	(void)bd;

    LFS_FLASHBD_TRACE("lfs_flashbd_destroy -> %d", 0);
    return 0;
}

int lfs_flashbd_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE("lfs_flashbd_read(%p, "
                "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)cfg, block, off, buffer, size);
    lfs_flashbd_t *bd = cfg->context;

    // check if read is valid
    LFS_ASSERT(off  % cfg->read_size == 0);
    LFS_ASSERT(size % cfg->read_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    // read data
	bk_flash_read_bytes(cfg->block_size*block+off+bd->start_addr,buffer,size);

    LFS_FLASHBD_TRACE("lfs_flashbd_read -> %d", 0);
    return 0;
}

int lfs_flashbd_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE("lfs_flashbd_prog(%p, "
                "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)cfg, block, off, buffer, size);
    lfs_flashbd_t *bd = cfg->context;

    // check if write is valid
    LFS_ASSERT(off  % cfg->prog_size == 0);
    LFS_ASSERT(size % cfg->prog_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    // progflash data
	bk_flash_write_bytes(cfg->block_size*block+off+bd->start_addr,buffer,size);

    LFS_FLASHBD_TRACE("lfs_flashbd_prog -> %d", 0);
    return 0;
}

int lfs_flashbd_erase(const struct lfs_config *cfg, lfs_block_t block) {
    LFS_FLASHBD_TRACE("lfs_flashbd_erase(%p, 0x%"PRIx32")", (void*)cfg, block);
    lfs_flashbd_t *bd = cfg->context;

    // check if erase is valid
    LFS_ASSERT(block < cfg->block_count);

    // erase
	bk_flash_erase_sector(cfg->block_size*block+bd->start_addr);

    LFS_FLASHBD_TRACE("lfs_flashbd_erase -> %d", 0);
    return 0;
}

int lfs_flashbd_sync(const struct lfs_config *cfg) {
    LFS_FLASHBD_TRACE("lfs_flashbd_sync(%p)", (void*)cfg);
    // sync does nothing
    (void)cfg;
    LFS_FLASHBD_TRACE("lfs_flashbd_sync -> %d", 0);
    return 0;
}

#ifdef CONFIG_SPI_MST_FLASH

// Modified by TUYA Start
extern bk_err_t sflash_erase(uint32_t addr, uint32_t size);
extern bk_err_t sflash_read(uint32_t addr, uint8_t *buff, uint32_t size);
extern bk_err_t sflash_write(uint32_t addr, const uint8_t *buff, uint32_t size);
// Modified by TUYA End

//keep sync with spi_flash.c
#define SPI_ID 1
#define SPI_BAUD_RATE	15000000

static int spi_inited = 0;

int lfs_spi_flashbd_init(void) {
	int ret;
	spi_config_t config = {0};

	if (spi_inited)
		return 0;
	spi_inited = 1;

	ret = bk_spi_driver_init();
	if (ret)
		return ret;

	config.role = SPI_ROLE_MASTER;
	config.bit_width = SPI_BIT_WIDTH_8BITS;
	config.polarity = 0;
	config.phase = 0;
	config.wire_mode = SPI_4WIRE_MODE;
	config.baud_rate = SPI_BAUD_RATE;
	config.bit_order = SPI_MSB_FIRST;
#if (CONFIG_SPI_BYTE_INTERVAL)
	config.byte_interval = 1;
#endif
#if CONFIG_SPI_DMA
	config.dma_mode = 1;
	config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_DTCM);
	config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_DTCM);
	config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
	config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
#endif
	ret = bk_spi_init(SPI_ID, &config);

	return ret;
}

int lfs_spi_flashbd_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_read(%p, "
                "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)cfg, block, off, buffer, size);
    lfs_flashbd_t *bd = cfg->context;

    // check if read is valid
    LFS_ASSERT(off  % cfg->read_size == 0);
    LFS_ASSERT(size % cfg->read_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    // read data
// Modified by TUYA Start
    sflash_read(cfg->block_size*block+off+bd->start_addr, buffer, size);
// Modified by TUYA End

    LFS_FLASHBD_TRACE("lfs_spi_flashbd_read -> %d", 0);
    return 0;
}

int lfs_spi_flashbd_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_prog(%p, "
                "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)cfg, block, off, buffer, size);
    lfs_flashbd_t *bd = cfg->context;

    // check if write is valid
    LFS_ASSERT(off  % cfg->prog_size == 0);
    LFS_ASSERT(size % cfg->prog_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    // progflash data
// Modified by TUYA Start
    sflash_write(cfg->block_size*block + bd->start_addr + off, buffer, size);
// Modified by TUYA End

    LFS_FLASHBD_TRACE("lfs_spi_flashbd_prog -> %d", 0);
    return 0;
}

int lfs_spi_flashbd_erase(const struct lfs_config *cfg, lfs_block_t block) {
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_erase(%p, 0x%"PRIx32")", (void*)cfg, block);
    lfs_flashbd_t *bd = cfg->context;

    // check if erase is valid
    LFS_ASSERT(block < cfg->block_count);

    // erase
// Modified by TUYA Start
    sflash_erase(cfg->block_size*block+bd->start_addr, cfg->block_size);
// Modified by TUYA End

    LFS_FLASHBD_TRACE("lfs_spi_flashbd_erase -> %d", 0);
    return 0;
}

int lfs_spi_flashbd_sync(const struct lfs_config *cfg) {
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_sync(%p)", (void*)cfg);
    // sync does nothing
    (void)cfg;
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_sync -> %d", 0);
    return 0;
}

#else

int lfs_spi_flashbd_init(void)
{
	return -1;
}

int lfs_spi_flashbd_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
	return -1;
}

int lfs_spi_flashbd_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
	return -1;
}

int lfs_spi_flashbd_erase(const struct lfs_config *cfg, lfs_block_t block)
{
	return -1;
}


int lfs_spi_flashbd_sync(const struct lfs_config *cfg)
{
	return -1;
}

#endif

// Modified by TUYA Start
#ifdef CONFIG_QSPI

extern bk_err_t qflash_erase(uint32_t addr, uint32_t size);
extern bk_err_t qflash_read(uint32_t addr, uint8_t *buff, uint32_t size);
extern bk_err_t qflash_write(uint32_t addr, const uint8_t *buff, uint32_t size);

int lfs_qspi_flashbd_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_read(%p, "
                "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)cfg, block, off, buffer, size);
    lfs_flashbd_t *bd = cfg->context;

    // check if read is valid
    LFS_ASSERT(off  % cfg->read_size == 0);
    LFS_ASSERT(size % cfg->read_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    // read data
    qflash_read(cfg->block_size*block+off+bd->start_addr, buffer, size);

    LFS_FLASHBD_TRACE("lfs_spi_flashbd_read -> %d", 0);
    return 0;
}

int lfs_qspi_flashbd_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_prog(%p, "
                "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)cfg, block, off, buffer, size);
    lfs_flashbd_t *bd = cfg->context;

    // check if write is valid
    LFS_ASSERT(off  % cfg->prog_size == 0);
    LFS_ASSERT(size % cfg->prog_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    qflash_write(cfg->block_size*block + bd->start_addr + off, buffer, size);

    LFS_FLASHBD_TRACE("lfs_spi_flashbd_prog -> %d", 0);
    return 0;
}

int lfs_qspi_flashbd_erase(const struct lfs_config *cfg, lfs_block_t block) {
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_erase(%p, 0x%"PRIx32")", (void*)cfg, block);
    lfs_flashbd_t *bd = cfg->context;

    // check if erase is valid
    LFS_ASSERT(block < cfg->block_count);

    // erase
    qflash_erase(cfg->block_size*block+bd->start_addr, cfg->block_size);

    LFS_FLASHBD_TRACE("lfs_spi_flashbd_erase -> %d", 0);
    return 0;
}

int lfs_qspi_flashbd_sync(const struct lfs_config *cfg) {
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_sync(%p)", (void*)cfg);
    // sync does nothing
    (void)cfg;
    LFS_FLASHBD_TRACE("lfs_spi_flashbd_sync -> %d", 0);
    return 0;
}

#else

int lfs_qspi_flashbd_init(void)
{
	return -1;
}

int lfs_qspi_flashbd_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
	return -1;
}

int lfs_qspi_flashbd_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
	return -1;
}

int lfs_qspi_flashbd_erase(const struct lfs_config *cfg, lfs_block_t block)
{
	return -1;
}


int lfs_qspi_flashbd_sync(const struct lfs_config *cfg)
{
	return -1;
}

#endif
// Modified by TUYA End

