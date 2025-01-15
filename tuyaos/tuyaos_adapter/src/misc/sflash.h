#ifndef __SFLASH_H
#define __SFLASH_H


#include <components/system.h>
#include <os/os.h>
#include <driver/spi.h>
#include <driver/dma.h>

#define SPI_FLASH_ID SPI_ID_1

void sflash_printf_buff(const uint8_t *buff, uint32_t len, uint32_t num_per_line, bool space, bool end_new_line);
bk_err_t sflash_init(void);
bk_err_t sflash_deinit(void);
bk_err_t sflash_erase(uint32_t addr, uint32_t size);
bk_err_t sflash_read(uint32_t addr, uint8_t *buff, uint32_t size);
bk_err_t sflash_write(uint32_t addr, const uint8_t *buff, uint32_t size);


#endif  /*__SFLASH_H*/