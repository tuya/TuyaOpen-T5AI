#ifndef __QFLASH_H
#define __QFLASH_H


#include <components/system.h>
#include <os/os.h>
#include <driver/qspi.h>
#include <driver/qspi_flash.h>

bk_err_t qflash_init(void);
bk_err_t qflash_deinit(void);
bk_err_t qflash_erase(uint32_t addr, uint32_t size);
bk_err_t qflash_read(uint32_t addr, uint8_t *buff, uint32_t size);
bk_err_t qflash_write(uint32_t addr, const uint8_t *buff, uint32_t size);


#endif  /*__QFLASH_H*/
