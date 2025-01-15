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
#include <common/bk_include.h>
#include <driver/otp_types.h>
#include <driver/vault.h>
#include <os/mem.h>
#include "otp_driver.h"
#include "otp_hal.h"

typedef struct {
	otp_hal_t hal;
} otp_driver_t;

/*the smallest unit length for configuration*/

/*              permission       secure_range             mask         */
/*    otp1        4 Byte            256 Byte             32 Byte       */
/*    otp2        128 Byte           NA                  128 Byte      */
/*    puf         4   Byte           32 Byte             32 Byte       */


otp_item_t otp_apb_map[] = {
	/* otp_id_t name,          allocated_size, offset,      privilege*/

	{OTP_MEMORY_CHECK_MARK,          96,        0x0,       OTP_READ_WRITE},
	{OTP_AES_KEY,                    32,        0x60,      OTP_READ_WRITE},
	/*reserved 128,0x80*/
	{OTP_MODEL_ID,                   4,         0x100,     OTP_READ_WRITE},
	{OTP_MODEL_KEY,                  16,        0x104,     OTP_READ_WRITE},
	{OTP_ARM_DEVICE_ID,              4,         0x114,     OTP_READ_WRITE},
	{OTP_DEVICE_ROOT_KEY,            16,        0x118,     OTP_READ_WRITE},
	{OTP_BL1_BOOT_PUBLIC_KEY_HASH,   32,        0x128,     OTP_READ_WRITE},
	{OTP_BL2_BOOT_PUBLIC_KEY_HASH,   32,        0x148,     OTP_READ_WRITE},
	{OTP_ARM_LCS,                    4,         0x168,     OTP_READ_WRITE},
	{OTP_LOCK_CONTROL,               4,         0x16C,     OTP_READ_WRITE},
	/*reserved 16,0x180*/
	{OTP_BL1_SECURITY_COUNTER,       4,         0x188,     OTP_READ_WRITE},
	/*reserved 116,0x18C*/
	{OTP_BL2_SECURITY_COUNTER,       64,        0x200,     OTP_READ_WRITE},
	{OTP_HUK,                        32,        0x240,     OTP_READ_WRITE},
	{OTP_IAK,                        32,        0x260,     OTP_READ_WRITE},
	{OTP_IAK_LEN,                    4,         0x280,     OTP_READ_WRITE},
	{OTP_IAK_TYPE,                   4,         0x284,     OTP_READ_WRITE},
	{OTP_IAK_ID,                     32,        0x288,     OTP_READ_WRITE},
	{OTP_BOOT_SEED,                  32,        0x2A8,     OTP_READ_WRITE},
	{OTP_LCS,                        4,         0x2C8,     OTP_READ_WRITE},
	{OTP_IMPLEMENTATION_ID,          32,        0x2CC,     OTP_READ_WRITE},
	{OTP_HW_VERSION,                 32,        0x2EC,     OTP_READ_WRITE},
	{OTP_VERIFICATION_SERVICE_URL,   32,        0x30C,     OTP_READ_WRITE},
	{OTP_PROFILE_DEFINITION,         32,        0x32C,     OTP_READ_WRITE},
	{OTP_ENTROPY_SEED,               64,        0x34C,     OTP_READ_WRITE},
	{OTP_SECURE_DEBUG_PK,            2,         0x38C,     OTP_READ_WRITE},
	{OTP_MAC_ADDRESS,                8,         0x3AC,     OTP_READ_WRITE},
	{OTP_VDDDIG_BANDGAP,             2,         0x3B4,     OTP_READ_WRITE},
	{OTP_DIA,                        2,         0x3B6,     OTP_READ_WRITE},
	{OTP_GADC_CALIBRATION,           4,         0x3B8,     OTP_READ_WRITE},
	{OTP_SDMADC_CALIBRATION,         4,         0x3BC,     OTP_READ_WRITE},
	{OTP_DEVICE_ID,                  8,         0x3C0,     OTP_READ_WRITE},
	{OTP_MEMORY_CHECK_VDDDIG,        4,         0x3C8,     OTP_READ_WRITE},
	{OTP_GADC_TEMPERATURE,           2,         0x3CC,     OTP_READ_WRITE},
	/*reserved 48,0x3CE*/
	{OTP_RESERVED,                   48,        0x3CE,     OTP_READ_WRITE},
	{OTP_CHIP_RESERVED,              2,         0x3FE,     OTP_READ_WRITE},//the chip reserved is for wifi, other modules can not use it
};

static const otp2_item_t otp_ahb_map[] = {
	{OTP_PHY_PWR,                    64,        0x0,       OTP_READ_WRITE},
	{OTP_RFCALI1,                    256,       0x40,      OTP_READ_WRITE},
	{OTP_RFCALI2,                    256,       0x140,     OTP_READ_WRITE},
	{OTP_RFCALI3,                    256,       0x240,     OTP_READ_WRITE},
	{OTP_RFCALI4,                    256,       0x340,     OTP_READ_WRITE},
};

static otp_driver_t s_otp = {0};


static void otp_sleep()
{
#if CONFIG_OTP_SLEEP
#if CONFIG_ATE_TEST
	return ;
#endif
#if !CONFIG_SPE
	otp_hal_sleep(&s_otp.hal);
#else
	otp_hal_deinit(&s_otp.hal);
#endif
#endif
}

static int otp_active()
{
#if CONFIG_ATE_TEST
	return 0;
#endif
#if !CONFIG_SPE
	return otp_hal_active(&s_otp.hal);
#else 
	return otp_hal_init(&s_otp.hal);
#endif
}

bk_err_t bk_otp_init()
{
	otp_hal_init(&s_otp.hal);
	return BK_OK;
}

bk_err_t bk_otp_deinit()
{
	otp_hal_deinit(&s_otp.hal);
	return BK_OK;
}

static uint32_t otp_read_otp(uint32_t location)
{
	return otp_hal_read_otp(&s_otp.hal,location);
}
static uint32_t otp2_read_otp(uint32_t location)
{
	return otp2_hal_read_otp(&s_otp.hal,location);
}

static void otp2_write_otp(uint32_t location,uint32_t value)
{
	otp2_hal_write_otp(&s_otp.hal, location, value);
}

static void otp_write_otp(uint32_t location,uint32_t value)
{
	otp_hal_write_otp(&s_otp.hal,location,value);
}

static int otp_check_write_empty(uint32_t location)
{
	if(otp_read_otp(location) != 0){
		return -1;
	} else {
		return 0;
	}
}

static uint32_t otp_read_status()
{
	return otp_hal_read_status(&s_otp.hal);
}

static uint32_t otp_get_failcnt()
{
	return otp_hal_get_failcnt(&s_otp.hal);
}

uint32_t otp_read_otp2_permission(uint32_t location)
{
	return otp_hal_read_otp2_permission(&s_otp.hal,location);
}

static void otp_set_lck_mask(uint32_t location)
{
	otp_hal_enable_mask_lck(&s_otp.hal);
}
static uint32_t otp_read_otp_mask(uint32_t location)
{
	return otp_hal_read_otp_mask(&s_otp.hal,location);
}
static void otp_zeroize_otp(uint32_t location)
{
	otp_hal_zeroize_otp(&s_otp.hal,location);
}
static void otp_zeroize_puf(uint32_t location)
{
	otp_hal_zeroize_puf(&s_otp.hal,location);
}
static uint32_t otp_get_otp_zeroized_flag(uint32_t location)
{
	return otp_hal_read_otp_zeroized_flag(&s_otp.hal,location);
}

uint32_t bk_otp_apb_read_permission(otp_id_t item)
{
	uint32_t location = otp_apb_map[item].offset / 4;
	uint32_t permission = otp_hal_read_otp_permission(&s_otp.hal,location) & otp_hal_read_otp_mask(&s_otp.hal,location);
	os_printf("item permission: 0x%x, mask: 0x%x\r\n",otp_hal_read_otp_permission(&s_otp.hal,location),otp_hal_read_otp_mask(&s_otp.hal,location));
	if(permission == 0xF) {
		return OTP_NO_ACCESS;
	} else if(permission == 0x3) {
		return OTP_READ_ONLY;
	} else if(permission == 0x0) {
		return OTP_READ_WRITE;
	} else {
		return BK_FAIL;
	}
}

uint32_t bk_otp_apb_write_permission(otp_id_t item, uint32_t permission)
{
	uint32_t location = otp_apb_map[item].offset / 4;
	uint32_t value;
	if(permission == OTP_READ_ONLY){
		value = 0x3;
	} else if(permission == OTP_NO_ACCESS){
		value = 0xF;
	} else{
		return BK_FAIL;
	}
	if(value < otp_hal_read_otp_permission(&s_otp.hal,location)){
		return BK_FAIL;
	}
	for(uint32_t location = otp_apb_map[item].offset / 4; location <= (otp_apb_map[item].offset+otp_apb_map[item].allocated_size) / 4;++location){
		otp_hal_write_otp_permission(&s_otp.hal,location,value);
	}
	otp_apb_map[item].privilege = bk_otp_apb_read_permission(item);
	return otp_apb_map[item].privilege;
}

uint32_t bk_otp_apb_read_mask(otp_id_t item)
{
	uint32_t location = otp_apb_map[item].offset / 4;
	uint32_t mask = otp_hal_read_otp_mask(&s_otp.hal,location);
	if(mask == 0xF) {
		return OTP_NO_ACCESS;
	} else if(mask == 0x3) {
		return OTP_READ_ONLY;
	} else if(mask == 0x0) {
		return OTP_READ_WRITE;
	} else {
		return BK_FAIL;
	}
}

bk_err_t bk_otp_apb_write_mask(otp_id_t item, uint32_t mask)
{
	uint32_t location = otp_apb_map[item].offset / 4;
	uint32_t value;
	if(mask == OTP_READ_ONLY){
		value = 0x3;
	} else if(mask == OTP_NO_ACCESS){
		value = 0xF;
	} else if(mask == OTP_READ_WRITE){
		value = 0x0;
	} else {
		return BK_FAIL;
	}
	if(value < otp_hal_read_otp_mask(&s_otp.hal,location)){
		return BK_FAIL;
	}

	otp_hal_write_otp_mask(&s_otp.hal,location,value);
	return BK_OK;

}

/**
 * obtain APB OTP value in little endian with item ID:
 * 1. allowed start address of item not aligned
 * 2. allowed end address of item not aligned
 */
bk_err_t bk_otp_apb_read(otp_id_t item, uint8_t* buf, uint32_t size)
{
	if(OTP_APB_MAX <= item) {
		os_printf("item=%d,max=%d\r\n", item, OTP_APB_MAX);
		return BK_ERR_OTP_OPERATION_FAIL;
	}
	if(otp_apb_map[item].privilege == OTP_NO_ACCESS){
		return BK_ERR_NO_READ_PERMISSION;
	}
	if(size > otp_apb_map[item].allocated_size){
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	if(otp_active() == -1){
		otp_sleep();
		return BK_FAIL;
	}
	uint32_t location = otp_apb_map[item].offset / 4;
	uint32_t start = otp_apb_map[item].offset % 4;
	uint32_t value;
	int byte_index, word_index;

	/*read first word*/
	if(size / 4 < 1){
		value = otp_read_otp(location);
		if(value == 0xFFFFFFFF){
			otp_sleep();
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < size; ++byte_index){
			buf[byte_index] = (value >> (start + byte_index)*8) & 0xFF;
		}
		otp_sleep();
		return BK_OK;
	} else {
		value = otp_read_otp(location);
		if(value == 0xFFFFFFFF){
			otp_sleep();
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < 4 - start; ++byte_index){
			buf[byte_index] = (value >> (start + byte_index)*8) & 0xFF;
		}
	}

	/*read full word*/
	for(word_index = 1; word_index < size / 4;++word_index){
		value = otp_read_otp(location + word_index);
		if(value == 0xFFFFFFFF){
			otp_sleep();
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < 4;++byte_index){
			buf[word_index*4+byte_index-start] = (value >> (byte_index*8)) & 0xFF; // low bit in low address
		}
	}

	if(word_index * 4 == size) {
		otp_sleep();
		return BK_OK;
	}
	/*read tail word*/
	value = otp_read_otp(location + word_index);
	if(value == 0xFFFFFFFF){
		otp_sleep();
		return BK_ERR_NO_READ_PERMISSION;
	}
	for(byte_index = 0;byte_index < (size+start) % 4;++byte_index){
		buf[word_index*4+byte_index-start] = (value >> (byte_index*8)) & 0xFF;
	}
	otp_sleep();
	return BK_OK;
}

/**
 * update APB OTP value in little endian with item ID:
 * 1. allowed start address of item not aligned
 * 2. allowed end address of item not aligned
 * 3. check overwritable before write, return BK_ERR_OTP_CANNOT_WRTIE if failed
 * 4. verify value after write, return BK_ERR_OTP_UPDATE_NOT_EQUAL if failed
 */
bk_err_t bk_otp_apb_update(otp_id_t item, uint8_t* buf, uint32_t size)
{
	if(OTP_APB_MAX <= item) {
		os_printf("item=%d,max=%d\r\n", item, OTP_APB_MAX);
		return BK_ERR_OTP_OPERATION_FAIL;
	}

	if(otp_apb_map[item].privilege != OTP_READ_WRITE){
		return BK_ERR_NO_WRITE_PERMISSION;
	}
	if(size > otp_apb_map[item].allocated_size){
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	uint32_t location = otp_apb_map[item].offset / 4;
	uint32_t start = otp_apb_map[item].offset % 4;
	uint32_t value = 0;
	uint32_t mask = 0;
	uint32_t verify = 0;
	int byte_index, word_index;
	uint8_t* checkbuf = (uint8_t*)os_malloc(size*sizeof(uint8_t));
	if(checkbuf == NULL){
		return BK_ERR_NO_MEM;
	}

	bk_otp_apb_read(item,checkbuf,size);
	for(byte_index = 0;byte_index < size;++byte_index){
		if(buf[byte_index] != (buf[byte_index] | checkbuf[byte_index])){
			os_free(checkbuf);
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}
	if(otp_active() == -1){
		otp_sleep();
		return BK_FAIL;
	}
	/*update first word*/
	if(size / 4 < 1) {
		for(byte_index = 0;byte_index < size; ++byte_index){
			value |= (buf[byte_index] << (byte_index*8));
			mask  |= (0xFF << (byte_index*8));
		}
		value = value << (start * 8);
		mask  = mask << (start * 8);
		otp_write_otp(location, value);
		verify = otp_read_otp(location);
		os_free(checkbuf);
		otp_sleep();
		return ((verify & mask) != value) ? BK_ERR_OTP_UPDATE_NOT_EQUAL : BK_OK;
	} else {
		for(byte_index = 0;byte_index < 4 - start; ++byte_index){
			value |= (buf[byte_index] << (byte_index*8));
			mask  |= (0xFF << (byte_index*8));
		}
		value = value << (start * 8);
		mask  = mask << (start * 8);
		otp_write_otp(location, value);
		verify = otp_read_otp(location);
		if ((verify & mask) != value) {
			os_free(checkbuf);
			otp_sleep();
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}

	/*update full word*/
	for(word_index = 1; word_index < size / 4;++word_index){
		value = 0;
		for(byte_index = 0;byte_index < 4;++byte_index){
			value |= (buf[4*word_index-start+byte_index] << (byte_index*8));
		}
		otp_write_otp(location + word_index,value);
		verify = otp_read_otp(location + word_index);
		if (verify != value) {
			os_free(checkbuf);
			otp_sleep();
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}

	if(word_index * 4 == size) {
		otp_sleep();
		return BK_OK;
	}
	/*update tail word*/
	value = 0;
	mask  = 0;
	for(byte_index = 0;byte_index < (size+start) % 4; ++byte_index){
		value |= (buf[word_index*4-start+byte_index] << (byte_index*8));
		mask  |= (0xFF << (byte_index*8));
	}
	otp_write_otp(location + word_index,value);
	verify = otp_read_otp(location + word_index);
	os_free(checkbuf);
	otp_sleep();
	return ((verify & mask) != value) ? BK_ERR_OTP_UPDATE_NOT_EQUAL : BK_OK;
}

/**
 * obtain AHB OTP value with item ID:
 * 1. allowed start address of item not aligned
 * 2. allowed end address of item not aligned
 */
bk_err_t bk_otp_ahb_read(otp2_id_t item, uint8_t* buf, uint32_t size)
{
	if(otp_ahb_map[item].privilege == OTP_NO_ACCESS){
		return BK_ERR_NO_READ_PERMISSION;
	}
	if(size > otp_ahb_map[item].allocated_size){
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	if(otp_active() == -1){
		otp_sleep();
		return BK_FAIL;
	}
	uint32_t location = otp_ahb_map[item].offset / 4;
	uint32_t start = otp_ahb_map[item].offset % 4;
	uint32_t value;
	int byte_index, word_index;

	/*read first word*/
	if(size / 4 < 1){
		value = otp2_read_otp(location);
		if(value == 0xFFFFFFFF){
			otp_sleep();
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < size; ++byte_index){
			buf[byte_index] = (value >> (start + byte_index)*8) & 0xFF;
		}
		otp_sleep();
		return BK_OK;
	} else {
		value = otp2_read_otp(location);
		if(value == 0xFFFFFFFF){
			otp_sleep();
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < 4 - start; ++byte_index){
			buf[byte_index] = (value >> (start + byte_index)*8) & 0xFF;
		}
	}

	/*read full word*/
	for(word_index = 1; word_index < size / 4;++word_index){
		value = otp2_read_otp(location + word_index);
		if(value == 0xFFFFFFFF){
			otp_sleep();
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < 4;++byte_index){
			buf[word_index*4+byte_index-start] = (value >> (byte_index*8)) & 0xFF;
		}
	}

	if(word_index * 4 == size) {
		otp_sleep();
		return BK_OK;
	}
	/*read tail word*/
	value = otp2_read_otp(location + word_index);
	if(value == 0xFFFFFFFF){
		otp_sleep();
		return BK_ERR_NO_READ_PERMISSION;
	}
	for(byte_index = 0;byte_index < (size+start) % 4;++byte_index){
		buf[word_index*4+byte_index-start] = (value >> (byte_index*8)) & 0xFF;
	}
	otp_sleep();
	return BK_OK;
}

bk_err_t bk_otp_ahb_update(otp2_id_t item, uint8_t* buf, uint32_t size)
{
	if(otp_ahb_map[item].privilege != OTP_READ_WRITE){
		return BK_ERR_NO_WRITE_PERMISSION;
	}
	if(size > otp_ahb_map[item].allocated_size){
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	uint32_t location = otp_ahb_map[item].offset / 4;
	uint32_t start = otp_ahb_map[item].offset % 4;
	uint32_t value = 0;
	uint32_t mask = 0;
	uint32_t verify = 0;
	int byte_index, word_index;
	uint8_t* checkbuf = (uint8_t*)os_malloc(size*sizeof(uint8_t));
	if(checkbuf == NULL){
		return BK_ERR_NO_MEM;
	}

	bk_otp_ahb_read(item,checkbuf,size);
	for(byte_index = 0;byte_index < size;++byte_index){
		if(buf[byte_index] != (buf[byte_index] | checkbuf[byte_index])){
			os_free(checkbuf);
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}
	if(otp_active() == -1){
		otp_sleep();
		return BK_FAIL;
	}
	/*update first word*/
	if(size / 4 < 1) {
		for(byte_index = 0;byte_index < size; ++byte_index){
			value |= (buf[byte_index] << (byte_index*8));
			mask  |= (0xFF << (byte_index*8));
		}
		value = value << (start * 8);
		mask  = mask << (start * 8);
		otp2_write_otp(location, value);
		verify = otp2_read_otp(location);
		os_free(checkbuf);
		otp_sleep();
		return ((verify & mask) != value) ? BK_ERR_OTP_UPDATE_NOT_EQUAL : BK_OK;
	} else {
		for(byte_index = 0;byte_index < 4 - start; ++byte_index){
			value |= (buf[byte_index] << (byte_index*8));
			mask  |= (0xFF << (byte_index*8));
		}
		value = value << (start * 8);
		mask  = mask << (start * 8);
		otp2_write_otp(location, value);
		verify = otp2_read_otp(location);
		if ((verify & mask) != value) {
			os_free(checkbuf);
			otp_sleep();
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}

	/*update full word*/
	for(word_index = 1; word_index < size / 4;++word_index){
		value = 0;
		for(byte_index = 0;byte_index < 4;++byte_index){
			value |= (buf[4*word_index-start+byte_index] << (byte_index*8));
		}
		otp2_write_otp(location + word_index,value);
		verify = otp2_read_otp(location + word_index);
		if (verify != value) {
			os_free(checkbuf);
			otp_sleep();
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}

	if(word_index * 4 == size) {
		otp_sleep();
		return BK_OK;
	}
	/*update tail word*/
	value = 0;
	mask  = 0;
	for(byte_index = 0;byte_index < (size+start) % 4; ++byte_index){
		value |= (buf[word_index*4-start+byte_index] << (byte_index*8));
		mask  |= (0xFF << (byte_index*8));
	}
	otp2_write_otp(location + word_index,value);
	verify = otp2_read_otp(location + word_index);
	os_free(checkbuf);
	otp_sleep();
	return ((verify & mask) != value) ? BK_ERR_OTP_UPDATE_NOT_EQUAL : BK_OK;
}

bk_err_t bk_otp_read_random_number(uint32_t* value, uint32_t size)
{
	if(otp_active() == -1)
		return BK_FAIL;
	for(int i = 0; i < size; ++i){
		value[i] = otp_hal_read_random_number(&s_otp.hal);
	}
	otp_sleep();
	return BK_OK;
}

bk_err_t bk_enable_security_protect()
{
	/*TODO*/
	otp_hal_enable_security_protection(&s_otp.hal);
	return BK_OK;
}

/*Initial Phase,done at factory*/


static bk_err_t bk_otp_init_puf()
{
	if(otp_hal_read_enroll(&s_otp.hal) == 0xF){
		return BK_OK;
	} else if(otp_hal_read_enroll(&s_otp.hal) == 0x0){
		otp_hal_do_puf_enroll(&s_otp.hal);
	}
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("puf enrollment fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("puf enrollment has been executed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMlck is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	if(otp_hal_read_enroll(&s_otp.hal) != 0xF){
		OTP_LOGE("Error code:0x%x\r\n",otp_hal_read_status(&s_otp.hal));
		return BK_ERR_OTP_OPERATION_FAIL;
	}
	return BK_OK;
}

bk_err_t bk_otp_read_raw_entropy_output(uint32_t* value)
{
	otp_hal_set_fre_cont(&s_otp.hal, 1);
	*value = otp_hal_read_random_number(&s_otp.hal);
	otp_hal_set_fre_cont(&s_otp.hal, 0);
	return BK_OK;
}

static bk_err_t bk_otp_check_puf_quality()
{
	uint32_t flag = otp_hal_do_puf_quality_check(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("quality check fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGE("please init puf before quality check.\r\n");
		return BK_ERR_OTP_OPERATION_WARNING;
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	}
	return BK_OK;
}

static bk_err_t bk_otp_check_puf_health()
{
	uint32_t flag = otp_hal_do_puf_health_check(&s_otp.hal);
	while(flag & OTP_BUSY);
	flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("health check fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGE("please init puf before quality check.\r\n");
		return BK_ERR_OTP_OPERATION_WARNING;
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	}
	return BK_OK;
}

static bk_err_t bk_otp_do_auto_repair()
{
	uint32_t status = otp_hal_do_auto_repair(&s_otp.hal);
	uint32_t flag = status & 0x1F;
	uint32_t failcnt = (status & 0xF00) >> 8;

	if(failcnt != 0){
		OTP_LOGE("The total number of failed address is 0x%x.\r\n",failcnt);
	}
	if(failcnt > 4){
		return failcnt;
	}
	if(flag & OTP_ERROR){
		OTP_LOGE("auto repair fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("auto repair has been previously executed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	return BK_OK;
}

static bk_err_t bk_otp_enable_shuffle_read()
{
	otp_hal_set_flag(&s_otp.hal,0x99);
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("enable shuffle read fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("shuffle read has been re-programmed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	volatile int delay = 1000;
	while(otp_hal_read_shfren(&s_otp.hal) != 0xF && delay){
		--delay;
	}
	otp_hal_deinit(&s_otp.hal);
	if(otp_hal_init(&s_otp.hal) != BK_OK){
		return BK_ERR_OTP_INIT_FAIL;
	}
	while(otp_hal_check_busy(&s_otp.hal));
	return BK_OK;
}

static bk_err_t bk_otp_enable_shuffle_write()
{
	otp_hal_set_flag(&s_otp.hal,0xc2);
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	while(flag & OTP_BUSY);
	flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("enable shuffle write fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("shuffle write has been re-programmed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	volatile int delay = 1000;
	while(otp_hal_read_shfwen(&s_otp.hal) != 0xF && delay){
		--delay;
	}
	otp_hal_deinit(&s_otp.hal);
	if(otp_hal_init(&s_otp.hal) != BK_OK){
		return BK_ERR_OTP_INIT_FAIL;
	}
	while(otp_hal_check_busy(&s_otp.hal));
	return BK_OK;
}

static bk_err_t bk_otp_set_ptc_page(uint32_t page)
{
	otp_hal_set_ptc_page(&s_otp.hal,page);
	return BK_OK;
}

static bk_err_t bk_otp_test_page(uint32_t start, uint32_t end)
{
	uint32_t data = 0x5A5A5A5A;
	uint8_t bit = 0x1;
	for(uint32_t i = start;i < end;i++){
		otp_hal_write_test_row(&s_otp.hal,i,data);
		if(otp_hal_read_test_row(&s_otp.hal,i) != data){
			OTP_LOGE("test row %d = 0x%x,should be 0x%x.\r\n",i,otp_hal_read_test_row(&s_otp.hal,i),data);
			return BK_ERR_OTP_OPERATION_ERROR;
		}
		for(uint32_t j = 0; j < 4;j++){
			otp_hal_set_ptc_page(&s_otp.hal,j);
			otp_hal_write_test_column(&s_otp.hal,i,bit);
			if(otp_hal_read_test_column(&s_otp.hal,i) != bit){
				OTP_LOGE("test column %d  in page %d = 0x%x,should be 0x%x.\r\n",i,otp_hal_read_test_column(&s_otp.hal,i),bit);
				return BK_ERR_OTP_OPERATION_ERROR;
			}
		}
	}
	return BK_OK;
}

static bk_err_t bk_otp_off_margin_read()
{
	otp_hal_enable_off_margin_read(&s_otp.hal);
	if(bk_otp_test_page(8,12) == BK_OK){
		otp_hal_disable_off_margin_read(&s_otp.hal);
		return BK_OK;
	}
	OTP_LOGE("off margin read fail.\r\n");
	return BK_ERR_OTP_OPERATION_ERROR;
}

static bk_err_t bk_otp_on_margin_read()
{
	otp_hal_enable_on_margin_read(&s_otp.hal);
	if(bk_otp_test_page(12,16) == BK_OK){
		otp_hal_disable_on_margin_read(&s_otp.hal);
		return BK_OK;
	}
	OTP_LOGE("on margin read fail.\r\n");
	return BK_ERR_OTP_OPERATION_ERROR;
}

static bk_err_t bk_otp_initial_off_check()
{
	uint32_t flag = otp_hal_init_off_check(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("initial_off_check fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	return BK_OK;
}

static bk_err_t bk_otp_enable_program_protect_function()
{
	if(otp_hal_read_pgmprt(&s_otp.hal) != 0xF){
		return BK_OK;
	}
	otp_hal_set_flag(&s_otp.hal,0xb6);
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("program_protect fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("program_protect has been previously executed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGW("PTM data is not correctly set.\r\n");
		return BK_OK;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	if(otp_hal_read_pgmprt(&s_otp.hal) != 0xF){
		OTP_LOGE("program_protect fail.\r\n");
		return BK_ERR_OTP_OPERATION_FAIL;
	}
	return BK_OK;
}

static bk_err_t bk_otp_enable_test_mode_lock_function()
{
	if(otp_hal_read_tmlck(&s_otp.hal) == 0xF){
		return BK_OK;
	}
	otp_hal_set_flag(&s_otp.hal,0x71);
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("test_mode_lock fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("test_mode_lock has been previously executed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	if(otp_hal_read_tmlck(&s_otp.hal) != 0xF){
		OTP_LOGE("test_mode_lock fail.\r\n");
		return BK_ERR_OTP_OPERATION_FAIL;
	}
	return BK_OK;
}

uint32_t bk_otp_fully_flow_test()
{
	uint32_t ret;
	uint32_t retry_count = 10;
	uint32_t fail_code = 0;
	/*test mode lck means test has been success*/
	if(otp_hal_read_tmlck(&s_otp.hal) == 0xF){
		return BK_OK;
	}

	ret = bk_otp_init_puf();
	if(ret != BK_OK){
		fail_code |= 1 << 0;
		goto exit;
	}
	ret = bk_otp_check_puf_quality();
	if(ret != BK_OK){
		fail_code |= 1 << 1;
		goto exit;
	}
	ret = bk_otp_check_puf_health();
	if(ret != BK_OK){
		fail_code |= 1 << 2;
		goto exit;
	}
	ret = bk_otp_do_auto_repair();
	if(ret != BK_OK){
		fail_code |= 1 << 3;
		if (ret > 0)
			fail_code |= ret << 12;
		goto exit;
	}

	while(otp_hal_read_shfwen(&s_otp.hal) != 0xF && retry_count){
		ret = bk_otp_enable_shuffle_write();
		--retry_count;
		if(ret != BK_OK){
			fail_code |= 1 << 4;
			goto exit;
		}
	}
	if(otp_hal_read_shfwen(&s_otp.hal) != 0xF){
		OTP_LOGE("enable shuffle write fail.\r\n");
		ret = BK_ERR_OTP_OPERATION_FAIL;
		goto exit;
	}

	retry_count = 10;
	while(otp_hal_read_shfren(&s_otp.hal) != 0xF && retry_count){
		ret = bk_otp_enable_shuffle_read();
		--retry_count;
		if(ret != BK_OK){
			fail_code |= 1 << 5;
			goto exit;
		}
	}

	if(otp_hal_read_shfren(&s_otp.hal) != 0xF){
		OTP_LOGE("enable shuffle read fail.\r\n");
		ret = BK_ERR_OTP_OPERATION_FAIL;
		goto exit;
	}

	ret = bk_otp_test_page(0,8);
	if(ret != BK_OK){
		fail_code |= 1 << 6;
		goto exit;
	}
	ret = bk_otp_off_margin_read();
	if(ret != BK_OK){
		fail_code |= 1 << 7;
		goto exit;
	}
	ret = bk_otp_on_margin_read();
	if(ret != BK_OK){
		fail_code |= 1 << 8;
		goto exit;
	}
	ret = bk_otp_initial_off_check();
	if(ret != BK_OK){
		fail_code |= 1 << 9;
		goto exit;
	}
	ret = bk_otp_enable_program_protect_function();
	if(ret != BK_OK){
		fail_code |= 1 << 10;
		goto exit;
	}
	ret = bk_otp_enable_test_mode_lock_function();
	if(ret != BK_OK){
		fail_code |= 1 << 11;
		goto exit;
	}

exit:
	switch (ret)
	{
		case BK_ERR_OTP_OPERATION_ERROR:
			fail_code |= 1 << 16;
			break;

		case BK_ERR_OTP_OPERATION_WRONG:
			fail_code |= 1 << 17;
			break;

		case BK_ERR_OTP_OPERATION_FORBID:
			fail_code |= 1 << 18;
			break;

		case BK_ERR_OTP_OPERATION_FAIL:
			fail_code |= 1 << 19;
			break;

		case BK_ERR_OTP_INIT_FAIL:
			fail_code |= 1 << 20;
			break;

		default:
			break;
	}
	return fail_code;
}
