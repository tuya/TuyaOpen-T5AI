/*
 * bl_extflash.c
 *
 *  Created on: 2017-4-27
 *      Author: gang.cheng
 */

#include "bl_extflash.h"
#include "system.h"
#include "bl_spi.h"
#include "type.h"
#include <stdbool.h>        // bool
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "bl_uart.h"
#include "partitions_gen.h"
#include "hal/hal_common.h"

#define FLASH_IS_EMPTY_FLAG_ADDR  0x110
#define READ_RD_STATUS_TIME		0X05


uint8_t ext_flash_is_empty(void)
{
	uint8_t status;

	uint8_t buf[20];
	status = ext_flash_rd_data(0x110,buf,8);
	if(status != 0)
	{
		return 0;
	}
	
	if( 0 == memcmp(buf,CHIP_NAME_STR,6))
	//if((buf[0] == 'B') && (buf[1] == 'K')&&(buf[2] == '3')&&(buf[3] == '2')&&(buf[4] == '8')&&(buf[5] == '1'))
	{
//		bl_printf("flash not empty!!\r\n");
		if(buf[6] < 0x05 || buf[6] == 0xff)
		{
			buf[6] = 0x05;
		}
		return buf[6];
	}else
	{
//		bl_printf("flash  empty!!\r\n");
		return 0;
	}

}

__inline static void ext_flash_unlock()
{
	//Beken_Gpio_Set(GPIOB_0,1);
	//Beken_Gpio_Set(GPIOB_1,1);
}

__inline static void ext_flash_lock()
{
	//Beken_Gpio_Set(GPIOB_0,0);
	//Beken_Gpio_Set(GPIOB_1,1);
}




 u8 ext_flash_wr_enable(uint8_t enable)
{
		uint8_t cmd;
		u8 status = 0;
		if(enable == 1)
		{
			cmd = WR_ENABLE_CMD;
		}else
		{
			cmd = WR_DISABLE_CMD;
		}
		status = spi_write_read(&cmd,1,NULL,0,0);

		return status;
}


uint16_t ext_flash_rd_status(void)
{
		uint8_t cmd[4];
		uint16_t status;

		u8 spi_status;
		uint8_t rbuf[2];
		cmd[0] = RD_STATUS_REG_CMD;
		cmd[1] = 0;
		spi_status = spi_write_read(cmd,2,&rbuf[0],1,1);

		if(spi_status == 0)
		{
			status = (rbuf[0] & 0x01);
		}else
		{
			status = SPI_OP_T_OUT;//(rbuf[0] & 0x01);// spi_status ;
		}
	//	bl_printf("rbuf[0] = %x \r\n",rbuf[0]);
		return status;
}

uint32_t ext_flash_rd_identify(void)
{
		uint8_t cmd;
		uint32_t id;

		uint8_t rbuf[3];
		cmd = READ_IDENTIFICATION_CMD;
		ext_flash_unlock();
		spi_write_read(&cmd,1,&rbuf[0],3,1);
		ext_flash_lock();
		id = (rbuf[0] | (rbuf[1] << 8) | (rbuf[2] << 16));

		return id ;
}

uint8_t  ext_flash_erase_one_sector(uint32_t address)
{

		uint8_t status = 0;
		uint8_t w_buf[4];
	    uint32_t Address = address  & (~FLASH_ERASE_SECTOR_SIZE_MASK);
//	    bl_printf("erase Address = 0x%08x\r\n",Address);
		w_buf[0] = SECTOR_ERASE_CMD;

		w_buf[1] = (Address >> 16 ) & 0XFF;
	    w_buf[2] = (Address >> 8 ) & 0XFF;
	    w_buf[3] = (Address >> 0 ) & 0XFF;

	 //   enable_pwm1_pt2();
	    status = ext_flash_rd_status();
	    set_5mstime_cnt(0);
		while((0x03 << 0) & (status))
		{
			if(get_5mstime_cnt() >= 200)
			{
			//	disble_pwm1_pt2();
				//status = 1;
				return status;
			}
			status = ext_flash_rd_status();
		}

		status = ext_flash_wr_enable(1);
		status = spi_write_read(w_buf,4,NULL ,0,0);
		// status = ext_flash_wr_enable(0);

		set_5mstime_cnt(0);
		status = ext_flash_rd_status();
		while(((0x03 << 0) & (status)))
		{
			if(get_5mstime_cnt() >= 200)
			{
			//	disble_pwm1_pt2();
				//status = 1;
				return status;
			}
			status = ext_flash_rd_status();
		}
	//	disble_pwm1_pt2();

		return status;

}

uint8_t  ext_flash_erase_sector_or_block_size(uint8 size_cmd,uint32_t address)
{

		uint8_t status = 0;
		uint8_t w_buf[4];
	    uint32_t Address ;

	    Address = address;
	    w_buf[0] = size_cmd;
		w_buf[1] = (Address >> 16 ) & 0XFF;
	    w_buf[2] = (Address >> 8 ) & 0XFF;
	    w_buf[3] = (Address >> 0 ) & 0XFF;
//	    bl_printf("erase Address = 0x%08x\r\n",Address);
	 //   enable_pwm1_pt2();
	    status = ext_flash_rd_status();
	    set_5mstime_cnt(0);
		while((0x03 << 0) & (status))
		{
			if(get_5mstime_cnt() >= 1000)
			{
			//	disble_pwm1_pt2();
				//status = 1;
				return status;
			}
			status = ext_flash_rd_status();
		}

		status = ext_flash_wr_enable(1);
		status = spi_write_read(w_buf,4,NULL ,0,0);
		// status = ext_flash_wr_enable(0);

		set_5mstime_cnt(0);
		status = ext_flash_rd_status();
		while(((0x03 << 0) & (status)))
		{
			if(get_5mstime_cnt() >= 1000)
			{
			//	disble_pwm1_pt2();
				//status = 1;
				return status;
			}
			status = ext_flash_rd_status();
		}
	//	disble_pwm1_pt2();

		return status;

}


uint8_t  ext_flash_erase_64k_block(uint32_t address)
{

		uint8_t status = 0;
		uint8_t w_buf[4];
	    uint32_t Address = address  & (~FLASH_ERASE_BLOCK64_SIZE_MASK);
//	    bl_printf("erase Address = 0x%08x\r\n",Address);
		w_buf[0] = BLOCK_ERASE_64K_CMD;

		w_buf[1] = (Address >> 16 ) & 0XFF;
	    w_buf[2] = (Address >> 8 ) & 0XFF;
	    w_buf[3] = (Address >> 0 ) & 0XFF;

	 //   enable_pwm1_pt2();
	    status = ext_flash_rd_status();
	    set_5mstime_cnt(0);
		while((0x03 << 0) & (status))
		{
			if(get_5mstime_cnt() >= 1000)
			{
			//	disble_pwm1_pt2();
				//status = 1;
				return status;
			}
			status = ext_flash_rd_status();
		}

		status = ext_flash_wr_enable(1);
		status = spi_write_read(w_buf,4,NULL ,0,0);
		// status = ext_flash_wr_enable(0);

		set_5mstime_cnt(0);
		status = ext_flash_rd_status();
		while(((0x03 << 0) & (status)))
		{
			if(get_5mstime_cnt() >= 1000)
			{
			//	disble_pwm1_pt2();
				//status = 1;
				return status;
			}
			status = ext_flash_rd_status();
		}
	//	disble_pwm1_pt2();

		return status;

}


uint32_t ext_flash_erase_section(uint32_t address, uint32_t size)
{
	uint32_t first_addr;
	uint32_t last_addr;
	uint32_t erase_len = 0;
	uint8_t status = 0;

	first_addr = address &( ~FLASH_ERASE_SECTOR_SIZE_MASK );

	last_addr = (address + size - 1) & ( ~FLASH_ERASE_SECTOR_SIZE_MASK );
//	bl_printf("erase frist_addr = 0x%08x,last_addr = 0x%08x\r\n",first_addr,last_addr);
	ext_flash_unlock();
	while((first_addr <= last_addr) &&(status == 0))
	{
//		bl_printf("erase_addr = 0x%08x\r\n",first_addr);
		status = ext_flash_erase_one_sector(first_addr);
		first_addr += FLASH_ERASE_SECTOR_SIZE;
		if(status == 0)
		{
			erase_len +=FLASH_ERASE_SECTOR_SIZE;
		}

	}
	ext_flash_lock();
	return erase_len;
}

uint32_t ext_flash_quickly_erase_section(uint32_t address, uint32_t size)
{
	uint32_t start = address & (~0xfff);
	uint32_t end = address + size;
	uint32_t status = STATUS_OK;

	while(start < end) {
		// printf("erase start addr=0x%x\n", start);
		if (!(start & 0xffff) && (start + 0x10000) < end) {
			status = ext_flash_erase_sector_or_block_size(BLOCK_ERASE_64K_CMD, start);
			start += 0x10000;
		} else if (!(start & 0x7fff) && (start + 0x8000) < (end)) {
			status = ext_flash_erase_sector_or_block_size(BLOCK_ERASE_32K_CMD, start);
			start += 0x8000;
		} else {
			status = ext_flash_erase_sector_or_block_size(SECTOR_ERASE_CMD, start);
			start += 0x1000;
		}
		if (status != STATUS_OK) {
			break;
		}
	}

	return status;
}

#if CONFIG_RANDOM_AES_UPGRADE_BL2
static void sec_boot_set_boot_flag(int boot_flag)
{
    if (boot_flag == BOOT_FLAG_PRIMARY) {
        AON_PMU_REG03_SET_BF_PRIMARY;
    } else {
        AON_PMU_REG03_CLR_BF_PRIMARY;
    }
}

uint8_t hal_write_preferred_boot_flag(const BOOT_FLAG flag)
{
	uint8_t ret = HAL_OK;
	uint8_t data[32] = {0};

	if ((flag != BOOT_FLAG_PRIMARY) && (flag != BOOT_FLAG_SECONDARY)) {
		printf("Parameter flag is invalid: %d!\n", flag);
		return HAL_ERR_BAD_PARAM;
	}

	ret = ext_flash_rd_data(CONFIG_BOOT_FLAG_PHY_PARTITION_OFFSET, (const uint8_t *)(&data[0]), sizeof(data));
	if (HAL_OK != ret) {
		printf("hal flash read failed!\n");
		return HAL_ERR_GENERIC;
	}
	// printf("magic=0x%02x%02x%02x%02x,data=0x%02x%02x%02x%02x, flag=0x%08x\r\n", data[3], data[2], data[1], data[0], data[7], data[6], data[5], data[4], flag);
	bl_memset(&data[4], 0x00, 4);
	bl_memcpy(&data[4], (uint8_t *)&flag, 1);
	// printf("magic=0x%02x%02x%02x%02x,data=0x%02x%02x%02x%02x\r\n", data[3], data[2], data[1], data[0], data[7], data[6], data[5], data[4]);
	ret = ext_flash_erase_one_sector(CONFIG_BOOT_FLAG_PHY_PARTITION_OFFSET);
	if (HAL_OK != ret) {
		printf("hal flash erase failed!\n");
		return HAL_ERR_GENERIC;
	}

	ret = ext_flash_wr_data(CONFIG_BOOT_FLAG_PHY_PARTITION_OFFSET, (const uint8_t *)(&data[0]), sizeof(data));
	if (HAL_OK != ret) {
		printf("hal flash write failed!\n");
		return HAL_ERR_GENERIC;
	}

	return ret;
}

uint8_t hal_read_preferred_boot_flag(BOOT_FLAG *flag)
{
	uint8_t ret = HAL_OK;
	boot_ctrl_data_t data = {0};

	if (!flag) {
		printf("Parameter flag is NULL!\n");
		return HAL_ERR_BAD_PARAM;
	}

	ret = ext_flash_rd_data(CONFIG_BOOT_FLAG_PHY_PARTITION_OFFSET,
							(uint8_t *)(&data),sizeof(boot_ctrl_data_t));
	if (HAL_OK != ret) {
		printf("hal flash read failed!\n");
		return HAL_ERR_GENERIC;
	}

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
#endif

uint32_t ext_flash_erase_chip(uint8_t time_outs) //CHIP_ERASE_CMD
{

			uint8_t status = 0;

			uint8_t w_buf[4];

//		    bl_printf("ext_flash_erase_chip \r\n");
			w_buf[0] = CHIP_ERASE_CMD; //;

		//	enable_pwm1_pt2();
			set_5mstime_cnt(0);
			status = ext_flash_rd_status();
			while((0x03 << 0) & (status))
			{
				if(get_5mstime_cnt() >= (time_outs * 200 ))
				{
				//	disble_pwm1_pt2();
				//	status = 1;
					return status;
				}
				status = ext_flash_rd_status();
			}
//			bl_printf("CNT 0 = %d \r\n",get_5mstime_cnt());
			status = ext_flash_wr_enable(1);
			status = spi_write_read(w_buf,1,NULL ,0,0);
			// status = ext_flash_wr_enable(0);

			set_5mstime_cnt(0);
			status = ext_flash_rd_status();
//			bl_printf("status = %d \r\n",status);
			while(((0x03 << 0) & (status)))
			{
				if(get_5mstime_cnt() >=(time_outs * 200))
				{
				//	disble_pwm1_pt2();
				//	status = 1;
					return status;
				}
				status = ext_flash_rd_status();
			}

//			bl_printf("CNT 1 = %d \r\n",get_5mstime_cnt());
		//	disble_pwm1_pt2();

	return status;
}

 uint8_t ext_flash_wr_data_in_page(uint32_t address,uint8_t *buf,uint32_t size)
{

		uint8_t status = 0;
		uint8_t w_buf[8];
		if(size == 0)
		{
			status = PARAM_ERROR;
			return status;
		}

		w_buf[0] = PAGE_PROGRAM_CMD;
		w_buf[1] = (address >> 16 ) & 0XFF;
	    w_buf[2] = (address >> 8 ) & 0XFF;
	    w_buf[3] = (address >> 0 ) & 0XFF;


	    set_5mstime_cnt(0);
	    status = ext_flash_rd_status();
		while((0x03 << 0) & (status))
		{
			if(get_5mstime_cnt() > 200)
			{
				//disble_pwm1_pt2();
				// status = ext_flash_rd_status();
				return status;
			}
			status = ext_flash_rd_status();
		}

		status = ext_flash_wr_enable(1);
#if 1
		spi_tx_start();

		status = spi_write_read_no_cs(w_buf,4,NULL,0,0);
		status = spi_write_read_no_cs(buf,size,NULL,0,0);
		spi_tx_end();
#else
		status = spi_write_read(w_buf,size + 4,NULL,0,0);

#endif

		// status = ext_flash_wr_enable(0);

		set_5mstime_cnt(0);
		status = ext_flash_rd_status();
		while((0x03 << 0) & (status))
		{
			if(get_5mstime_cnt() > 200)
			{
			//	disble_pwm1_pt2();
				// status = ext_flash_rd_status();
				return status;
			}
			status = ext_flash_rd_status();
		}

	//	disble_pwm1_pt2();
		return status;
}

uint8_t ext_flash_wr_data(uint32_t address,uint8_t *buf,uint32_t size)
{

	uint8_t  status;
	uint32_t pre_address;
	uint32_t post_address;
	uint32_t pre_len;
	uint32_t post_len;
	uint32_t page0;
	uint32_t page1;
//	bl_printf("%s address = 0x%08x\r\n",__func__,address);
	page0 = address & (~FLASH_PAGE_MASK);
	page1 = (address + size - 1) & (~FLASH_PAGE_MASK);

	//bl_printf("page0 = 0x%08x,page1 = 0x%08x \r\n",page0,page1);
	ext_flash_unlock();
	if(page0 != page1)
	{
			pre_address = address;
			pre_len = page1 - address;
		//	bl_printf("pre_address = 0x%08x,pre_len = 0x%08x \r\n",pre_address,pre_len);
			status = ext_flash_wr_data_in_page(pre_address,buf,pre_len);

			post_address = page1;
			post_len = address + size - page1;
			status = ext_flash_wr_data_in_page(post_address,buf + pre_len,post_len);
	//		bl_printf("post_address = 0x%08x,post_len = 0x%08x \r\n",post_address,post_len);

	}
	else
	{
			status= ext_flash_wr_data_in_page(address,buf,size);
	}
	ext_flash_lock();

	return status;
}


static uint8_t ext_flash_rd_data_in_page(uint32_t address,uint8_t *buf,uint32_t size)
{


		uint8_t status = 0;
		uint8_t w_buf[4];
	//	bl_printf("%s\r\n",__func__);
		w_buf[0] = RD_DATA_CMD;
		w_buf[1] = (address >> 16 ) & 0XFF;
	    w_buf[2] = (address >> 8 ) & 0XFF;
	    w_buf[3] = (address >> 0 ) & 0XFF;

	//    enable_pwm1_pt2();
	    set_5mstime_cnt(0);
	    status = ext_flash_rd_status();
		while((0x03 << 0) & (status))
		{

			if(get_5mstime_cnt() > 200)
			{
			//	disble_pwm1_pt2();
				//status = ext_flash_rd_status();
				return status;
			}
			status = ext_flash_rd_status();
		}
	//	disble_pwm1_pt2();
	//	status = spi_write_read(w_buf,4,buf,size,4);
		status = spi_write_read_4K_R(w_buf,4,buf,size,4);


		return status;
}

static uint8_t ext_flash_rd_data_in_page_for_crc(uint32_t address,uint8_t *buf,uint32_t size)
{


		uint8_t status = 0;
		uint8_t w_buf[4];
	//	bl_printf("%s\r\n",__func__);
		w_buf[0] = RD_DATA_CMD;
		w_buf[1] = (address >> 16 ) & 0XFF;
	    w_buf[2] = (address >> 8 ) & 0XFF;
	    w_buf[3] = (address >> 0 ) & 0XFF;


	    set_5mstime_cnt(0);
	    status = ext_flash_rd_status();
		while((0x03 << 0) & (status))
		{

			if(get_5mstime_cnt() > 200)
			{
			//	disble_pwm1_pt2();
				//status = ext_flash_rd_status();
				return status;
			}
			status = ext_flash_rd_status();
		}

		status = spi_write_read_4K_R_for_crc(w_buf,4,buf,size,4);


		return status;
}

uint8_t  ext_flash_rd_data(uint32_t address,uint8_t *buf,uint32_t size)
{
	uint8_t status = 0;
	uint32_t pre_address;
	uint32_t post_address;
	uint32_t pre_len;
	uint32_t post_len;
	uint32_t page0;
	uint32_t page1;
//	bl_printf("%s\r\n",__func__);
	if((buf == NULL) ||( size == 0))
	{
		status = 1;
		return status;
	}
	page0 = address &(~FLASH_PAGE_MASK);
	page1 = (address + size - 1) &(~FLASH_PAGE_MASK);
	//bl_printf("page0 = 0x%08x,page1 = 0x%08x \r\n",page0,page1);
	ext_flash_unlock();
	if(page0 != page1)
	{
		pre_address = address;
		pre_len = page1 - address;
		status = ext_flash_rd_data_in_page(pre_address, buf, pre_len);

	//	bl_printf("pre_address = 0x%08x,pre_len = 0x%08x \r\n",pre_address,pre_len);

		post_address = page1;
		post_len = address + size - page1;
		status = ext_flash_rd_data_in_page(post_address,(buf + pre_len), post_len);
	//	bl_printf("post_address = 0x%08x,post_len = 0x%08x \r\n",post_address,post_len);

	}
	else
	{
		status = ext_flash_rd_data_in_page(address ,buf , size );

	}
	ext_flash_lock();

	return 0;
	//return status;
}


uint8_t  ext_flash_rd_data_for_crc(uint32_t address,uint8_t *buf,uint32_t size)
{
	uint8_t status = 0;
	uint32_t pre_address;
	uint32_t post_address;
	uint32_t pre_len;
	uint32_t post_len;
	uint32_t page0;
	uint32_t page1;
//	bl_printf("%s\r\n",__func__);
	if((buf == NULL) ||( size == 0))
	{
		status = 1;
		return status;
	}
	page0 = address &(~FLASH_PAGE_MASK);
	page1 = (address + size - 1) &(~FLASH_PAGE_MASK);
	//bl_printf("page0 = 0x%08x,page1 = 0x%08x \r\n",page0,page1);
	ext_flash_unlock();
	if(page0 != page1)
	{
		pre_address = address;
		pre_len = page1 - address;
		status = ext_flash_rd_data_in_page_for_crc(pre_address, buf, pre_len);

	//	bl_printf("pre_address = 0x%08x,pre_len = 0x%08x \r\n",pre_address,pre_len);

		post_address = page1;
		post_len = address + size - page1;
		status = ext_flash_rd_data_in_page_for_crc(post_address,(buf + pre_len), post_len);
	//	bl_printf("post_address = 0x%08x,post_len = 0x%08x \r\n",post_address,post_len);

	}
	else
	{
		status = ext_flash_rd_data_in_page_for_crc(address ,buf , size );

	}
	ext_flash_lock();

	return 0;
	//return status;
}




void test_flash(void)
{
	uint8_t buf[256];
	uint16_t i;
	ad_printf("test_flash\r\n");
	ext_flash_erase_section(0X1000,0x1000);
	delay(10000);
	ad_printf("ext_flash_wr_data 0\r\n");
//	ext_flash_rd_data(00,buf,32);
	for( i = 0 ; i < 256 ; i ++)
	{
		buf[i] = i;
	//	bl_printf("buf[%d] = 0x%x\r\n",i,buf[i]);
	}
//	ext_flash_wr_data(0x1000 ,buf,256);
/*	bl_printf("set data \r\n");
	for( i = 0 ; i < 32 ; i ++)
		{
			buf[i] = i;
			bl_printf("buf[%d] = 0x%x\r\n",i,buf[i]);
		}
	bl_printf("ext_flash_wr_data \r\n");

	memset(buf,0,256);
	*/
	    i = 0;
		for( i = 0 ; i < 16 ; i ++)
		{
			ext_flash_wr_data(0x1000 + i * 256,buf,256);
			//bl_printf("buf[%d] = 0x%x\r\n",i,buf[i]);
		}
		ad_printf("test_flash end 0\r\n");
}
