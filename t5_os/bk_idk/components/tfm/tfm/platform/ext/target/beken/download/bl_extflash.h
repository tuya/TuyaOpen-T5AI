/*
 * bl_extflash.h
 *
 *  Created on: 2017-4-27
 *      Author: gang.cheng
 */

#ifndef BL_EXTFLASH_H_
#define BL_EXTFLASH_H_


#include "type.h"
#include <stdint.h>
///Base address of Flash on
#define EXT_FLASH_BASE_ADDR        					 0x00001000
#define EXT_FLASH_END_ADDR          				 0x0007FFFF  //256 * 2 + 3 //25 * sizeof(struct step_info_t) + 3//

#define FLASH_TOTAL_SIZE                   			 0x00080000  // 512K
#define FLASH_ERASE_SECTOR_SIZE						 0x1000   //4K
#define FLASH_ERASE_BLOCK32_SIZE					 0x8000   //32K
#define FLASH_ERASE_BLOCK64_SIZE					 0x10000   //64K
#define FLASH_ERASE_CHIP_SIZE					     FLASH_TOTAL_SIZE   //64K

#define FLASH_ERASE_SECTOR_SIZE_MASK				 (FLASH_ERASE_SECTOR_SIZE - 1)
#define FLASH_ERASE_BLOCK32_SIZE_MASK				 (FLASH_ERASE_BLOCK32_SIZE - 1)
#define FLASH_ERASE_BLOCK64_SIZE_MASK				 (FLASH_ERASE_BLOCK64_SIZE - 1)

#define FLASH_PAGE_SIZE								 (0x100)
#define FLASH_PAGE_MASK								 (FLASH_PAGE_SIZE - 1)


#define WR_ENABLE_CMD						0x06
#define WR_DISABLE_CMD						0x04

#define RD_STATUS_REG_CMD					0x05
#define RD_STATUS_REG1_CMD					0x35

#define WR_STATUS_REG_CMD					0x01

#define RD_DATA_CMD							0x03
#define RD_DATA_FAST_CMD					0x0B
#define PAGE_PROGRAM_CMD					0x02
#define SECTOR_ERASE_CMD					0x20
#define BLOCK_ERASE_32K_CMD					0x52
#define BLOCK_ERASE_64K_CMD					0xD8
#define CHIP_ERASE_CMD						0xC7
#define DEEP_POWER_DOWN_CMD					0xB9
#define RELEASE_FROM_DEEP_POWER_DOWN_CMD  	0xAB

#define GET_DEVICE_CMD						0x90
#define READ_IDENTIFICATION_CMD				0x9F



enum
{
	ERASE_4K_SIZE  = 1,
	ERASE_32K_SIZE  = 2,
	ERASE_64K_SIZE  = 3,

};

typedef enum boot_flag_t {
    BOOT_FLAG_INVALID  	= 0,
    BOOT_FLAG_PRIMARY  	= 1,
    BOOT_FLAG_SECONDARY = 2,
}BOOT_FLAG;

/**
 ****************************************************************************************
 * @brief Initialize flash driver.
 ****************************************************************************************
 */

void test_flash(void);
void ext_flash_init(void);


uint8_t ext_flash_is_empty(void);

uint16_t ext_flash_rd_status(void);
/**
 ****************************************************************************************
 * @brief   Identify the flash device.
 *
 * This function is used to read the flash device ID.
 *
 *
 ****************************************************************************************
 */


uint32_t ext_flash_rd_identify(void);
/**
 ****************************************************************************************
 * @brief   Erase a flash section.
 *
 * This function is used to erase a part of the flash memory.
 *
 * Note: callback parameter is not used
 *
 * @param[in]    address      Starting offset from the beginning of the flash device
 * @param[in]    size        Size of the portion of flash to erase
 * @return       erase_len    0 if operation can start successfully
 ****************************************************************************************
 */
uint32_t ext_flash_erase_section(uint32_t address, uint32_t size);

uint8_t  ext_flash_erase_sector_or_block_size(uint8 size_cmd,uint32_t address);

uint32_t ext_flash_erase_chip(uint8_t time_outs) ;//CHIP_ERASE_CMD


u8 ext_flash_erase_one_sector(uint32_t address);

uint32_t ext_flash_quickly_erase_section(uint32_t address, uint32_t size);

#if CONFIG_RANDOM_AES_UPGRADE_BL2
#define _CTRL_CTRL_MAGIC (0x4C725463)
typedef struct _boot_ctrl_data_t {
    uint32_t magic;
    uint32_t boot_flag;
} boot_ctrl_data_t;

uint8_t hal_write_preferred_boot_flag(const BOOT_FLAG flag);
uint8_t hal_read_preferred_boot_flag(BOOT_FLAG *flag);
#endif
/**
 ****************************************************************************************
 * @brief   Write a flash page.
 *
 * This function is used to write a part of the flash memory.
 *
 *
 * @param[in]    address      Starting offset from the beginning of the flash device
 * @param[in]    size      Size of the portion of flash to write
 * @param[in]    buffer      Pointer on data to write

 ****************************************************************************************
 */
u8 ext_flash_wr_data(uint32_t Offset,uint8_t *buf,uint32_t size);

uint8_t ext_flash_wr_data_in_page(uint32_t address,uint8_t *buf,uint32_t size);


/**
 ****************************************************************************************
 * @brief   Read a flash page.
 *
 * This function is used to read a part of the flash memory.
 *
 * @param[in]    offset      Starting offset from the beginning of the flash device
 * @param[in]    size      Size of the portion of flash to read
 * @param[out]   buffer      Pointer on data to read

 ****************************************************************************************
 */
u8 ext_flash_rd_data(uint32_t address,uint8_t *buf,uint32_t size);

uint8_t  ext_flash_rd_data_for_crc(uint32_t address,uint8_t *buf,uint32_t size);


u8 ext_flash_wr_enable(uint8_t enable);

#endif /* BL_EXTFLASH_H_ */
