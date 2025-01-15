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

#include <string.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "flash_ipc.h"
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define TAG		"flash_c"

#define LOCAL_TRACE    (1)

static bool s_flash_client_init = false;

static uint32_t         flash_socket_handle = 0;
static beken_mutex_t    flash_mutex = NULL;

static volatile flash_op_status_t s_flash_op_status = 0;

static const uint32_t crc32_table[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t calc_crc32_xor(uint32_t crc, const void *buf, size_t size)
{
    const uint8_t *p;

    p = (const uint8_t *)buf;
    crc = crc ^ ~0U;

    while (size--) {
        crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ ~0U;
}

uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, int len)
{
	while(len--)
	{
		crc = (crc >> 8)^(crc32_table[(crc^*buf++)&0xff]);
	}

	return crc;
}

bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode)
{
	return BK_FAIL;
}

#if CONFIG_FLASH_MB

#include <driver/mailbox_channel.h>
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

enum
{
	IPC_ERASE_COMPLETE = 0,
	IPC_ERASE_REQ,
	IPC_ERASE_ACK,
};

#if CONFIG_SYS_CPU0
#include <driver/aon_rtc.h>

static volatile uint32_t flash_erase_ipc_state = IPC_ERASE_COMPLETE;
#define FLASH_WAIT_ACK_TIMEOUT 5000

static bk_err_t send_pause_cmd(uint8_t log_chnl)
{
	mb_chnl_cmd_t  cmd_buf;
	cmd_buf.hdr.data = 0; /* clear hdr. */
	cmd_buf.hdr.cmd  = 1;
	flash_erase_ipc_state = IPC_ERASE_REQ;
	cmd_buf.param1 = (u32)&flash_erase_ipc_state;

	return mb_chnl_write(log_chnl, &cmd_buf);
}

static bk_err_t send_flash_op_prepare(void)			//CPU0 notify CPU1 before flash operation
{
	uint64_t us_start = 0;
	uint64_t us_end = 0;

	send_pause_cmd(MB_CHNL_FLASH);

	us_start = bk_aon_rtc_get_us();

	for(int i = 0; i < 2000; i++)
	{
#if CONFIG_CACHE_ENABLE
		flush_dcache((void *)&flash_erase_ipc_state, 4);
#endif
		if(flash_erase_ipc_state == IPC_ERASE_ACK)
		{
			break;
		}

		us_end = bk_aon_rtc_get_us();
		//wait ack time should not be more than 5 ms
		if((us_end - us_start) > FLASH_WAIT_ACK_TIMEOUT)
		{
			return BK_FAIL;
		}
	}

	return BK_OK;
}

static bk_err_t send_flash_op_finish(void)			//CPU0 notify CPU1 after flash operation
{
	flash_erase_ipc_state = IPC_ERASE_COMPLETE;

	return BK_OK;
}
#endif

#if CONFIG_SYS_CPU1

extern void lcd_hal_rgb_int_enable(bool is_sof_en, bool is_eof_en);
__attribute__((section(".iram"))) static bk_err_t cpu1_pause_handle(mb_chnl_cmd_t *cmd_buf)
{
	volatile uint32_t * stat_addr = (volatile uint32_t *)cmd_buf->param1;

#if CONFIG_CACHE_ENABLE
	flush_dcache((void *)stat_addr, 4);
#endif
#if CONFIG_LCD
        lcd_hal_rgb_int_enable(0, 0);
#endif
	//only puase cpu1 when flash erasing
	if(*(stat_addr) == IPC_ERASE_REQ)
	{
		bk_flash_set_operate_status(FLASH_OP_BUSY);
		*(stat_addr) = IPC_ERASE_ACK;
		while(*(stat_addr) != IPC_ERASE_COMPLETE)
		{
#if CONFIG_CACHE_ENABLE
			flush_dcache((void *)stat_addr, 4);
#endif
		}
		bk_flash_set_operate_status(FLASH_OP_IDLE);
	}

#if CONFIG_LCD
        lcd_hal_rgb_int_enable(0, 1);
#endif

	return BK_OK;
}
#endif

__attribute__((section(".iram"))) static void mb_flash_ipc_rx_isr(void *chn_param, mb_chnl_cmd_t *cmd_buf)
{
#if CONFIG_SYS_CPU1
	cpu1_pause_handle(cmd_buf);
#endif

	return;
}

static u8   flash_mb_init = 0;;

static bk_err_t mb_flash_ipc_init(void)
{
	if(flash_mb_init)
		return BK_OK;
	
	bk_err_t ret_code = mb_chnl_open(MB_CHNL_FLASH, NULL);

	if(ret_code != BK_OK)
	{
		return ret_code;
	}

	// call chnl driver to register isr callback;
	mb_chnl_ctrl(MB_CHNL_FLASH, MB_CHNL_SET_RX_ISR, (void *)mb_flash_ipc_rx_isr);

	flash_mb_init = 1;

	return ret_code;
}

static void mb_flash_ipc_deinit(void)
{
	mb_chnl_close(MB_CHNL_FLASH);
	flash_mb_init = 0;
}

#endif

__attribute__((section(".iram"))) bk_err_t bk_flash_set_operate_status(flash_op_status_t status)
{
	s_flash_op_status = status;
	return BK_OK;
}

__attribute__((section(".iram"))) flash_op_status_t bk_flash_get_operate_status(void)
{
	return s_flash_op_status;
}

bk_err_t bk_flash_driver_init(void)
{
	if (s_flash_client_init)
	{
		return BK_OK;
	}

#if CONFIG_FLASH_MB
	bk_err_t ret_code = mb_flash_ipc_init();

	if(ret_code != BK_OK)
		return ret_code;
#endif

	if(!rtos_is_scheduler_started())
		return BK_FAIL; // delay initialization.

	int ret = rtos_init_mutex(&flash_mutex);

	if(kNoErr != ret)
	{
		return BK_FAIL;
	}

	flash_socket_handle = mb_ipc_socket(IPC_GET_ID_PORT(FLASH_CLIENT), NULL);
	if(flash_socket_handle == 0)
	{
		#if LOCAL_TRACE
		BK_LOGE(TAG, "flash-drv-client create socket failed\r\n");
		#endif
		
		goto init_fail_exit;
	}
	
	ret = mb_ipc_connect(flash_socket_handle, IPC_GET_ID_CPU(FLASH_SERVER), IPC_GET_ID_PORT(FLASH_SERVER), 500);

	if(ret != 0)
	{
		#if LOCAL_TRACE
		BK_LOGE(TAG, "flash-drv-client connect failed %d\r\n", ret);
		#endif
		
		goto init_fail_exit;
	}

	s_flash_client_init = true;
	
	return BK_OK;
	
init_fail_exit:

	if(flash_socket_handle != 0)
	{
		mb_ipc_close(flash_socket_handle, FLASH_OPERATE_TIMEOUT);
		flash_socket_handle = 0;
	}
	
	rtos_deinit_mutex(&flash_mutex);
	flash_mutex = NULL;

	return BK_FAIL;
}

bk_err_t bk_flash_driver_deinit(void)
{
	if(!s_flash_client_init)
		return BK_OK;
	
	if(flash_socket_handle != 0)
	{
		mb_ipc_close(flash_socket_handle, FLASH_OPERATE_TIMEOUT);
		flash_socket_handle = 0;
	}
	
	rtos_deinit_mutex(&flash_mutex);
	flash_mutex = NULL;

	s_flash_client_init = false;

	return BK_OK;
}

bk_err_t bk_flash_erase_sector(uint32_t address)
{
	int  ret_val = BK_FAIL;
	int  line_num;
	
	if(bk_flash_driver_init() != BK_OK)
		return BK_FAIL;

	flash_cmd_t   cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	cmd_buff.addr = address;

	rtos_lock_mutex(&flash_mutex);

	int ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_ERASE_SECTOR, 
		(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto erase_sector_exit;
	}

	u8   user_cmd = INVALID_USER_CMD_ID;
	
	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff, 
		sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto erase_sector_exit;
	}

	if(user_cmd != FLASH_CMD_ERASE_SECTOR)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto erase_sector_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;

		goto erase_sector_exit;
	}

	ret_val = BK_OK;

erase_sector_exit:

	rtos_unlock_mutex(&flash_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_flash_erase_32k(uint32_t address)
{
	return BK_FAIL;
}

bk_err_t bk_flash_erase_block(uint32_t address)
{
	return BK_FAIL;
}

static bk_err_t flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	int  ret_val = BK_FAIL;
	int  line_num;
	
	if(size > 0xFFFF)
		return BK_FAIL;

	flash_cmd_t   cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	cmd_buff.addr = address;
	cmd_buff.len  = size;

	rtos_lock_mutex(&flash_mutex);

	int ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_READ, 
			(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto read_exit;
	}

	u8   user_cmd = INVALID_USER_CMD_ID;
	
	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff, 
			sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto read_exit;
	}

	if(user_cmd != FLASH_CMD_READ)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto read_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;

		goto read_exit;
	}

	if(cmd_buff.len != size)
	{
		line_num = __LINE__;
		ret = cmd_buff.len;

		goto read_exit;
	}

	#if CONFIG_CACHE_ENABLE
	flush_dcache(cmd_buff.buff, size);
	#endif
	memcpy(user_buf, cmd_buff.buff, size);

	u32  crc = calc_crc32(0, (const u8 *)user_buf, size);

	if(cmd_buff.crc != crc)
	{
		line_num = __LINE__;
		ret = crc;

		goto read_exit;
	}

	ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_READ_DONE, 
			(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto read_exit;
	}

	// u8   user_cmd = INVALID_USER_CMD_ID;

	// memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff, 
			sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);  // it is just a handshake. every send cmd to server must have a recv.

#if LOCAL_TRACE
	if(ret != sizeof(cmd_buff))
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, __LINE__, ret);
#endif

	ret_val = BK_OK;

read_exit:

	rtos_unlock_mutex(&flash_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	int  ret_val = BK_OK;
	u32  rd_len = 0;
	
	if(bk_flash_driver_init() != BK_OK)
		return BK_FAIL;

	while(size > FLASH_IPC_READ_SIZE)  // 4KB.
	{
		ret_val = flash_read_bytes(address + rd_len, user_buf + rd_len, FLASH_IPC_READ_SIZE);

		if(ret_val != BK_OK)
			return ret_val;

		rd_len += FLASH_IPC_READ_SIZE;
		size -= FLASH_IPC_READ_SIZE;
	}

	ret_val = flash_read_bytes(address + rd_len, user_buf + rd_len, size);

	return ret_val;

}
bk_err_t flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	int  ret_val = BK_FAIL;
	int  line_num;
	
	if(size > 0xFFFF)
		return BK_FAIL;

	flash_cmd_t   cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	cmd_buff.addr = address;
	cmd_buff.len  = size;
	cmd_buff.buff = (u8 *)user_buf;
	cmd_buff.crc  = calc_crc32(0, user_buf, size);

	rtos_lock_mutex(&flash_mutex);

	int ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_WRITE, 
		(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT + size / 200);  // flash write speed: write done 200 bytes in 1ms.

	if(ret != 0)
	{
		line_num = __LINE__;
		goto write_exit;
	}

	u8   user_cmd = INVALID_USER_CMD_ID;
	
	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff, 
		sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto write_exit;
	}

	if(user_cmd != FLASH_CMD_WRITE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto write_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;

		goto write_exit;
	}

	#if 0
	if(cmd_buff.len != size)
	{
		line_num = __LINE__;
		ret = cmd_buff.len;

		goto write_exit;
	}
	#endif

	ret_val = BK_OK;

write_exit:

	rtos_unlock_mutex(&flash_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	int  ret_val = BK_OK;
	u32  wr_len = 0;
	
	if(bk_flash_driver_init() != BK_OK)
		return BK_FAIL;


	while(size > FLASH_IPC_WRITE_SIZE)  // 4KB.
	{
		ret_val = flash_write_bytes(address + wr_len, user_buf + wr_len, FLASH_IPC_WRITE_SIZE);

		if(ret_val != BK_OK)
			return ret_val;

		wr_len += FLASH_IPC_WRITE_SIZE;
		size -= FLASH_IPC_WRITE_SIZE;
	}

	ret_val = flash_write_bytes(address + wr_len, user_buf + wr_len, size);

	return ret_val;

}

uint32_t bk_flash_get_id(void)
{
	return 0;
}

flash_line_mode_t bk_flash_get_line_mode(void)
{
	return FLASH_LINE_MODE_TWO;
}

flash_protect_type_t bk_flash_get_protect_type(void)
{
	return FLASH_PROTECT_ALL;
}

bk_err_t bk_flash_set_protect_type(flash_protect_type_t type)
{
	return BK_OK;
}

bool bk_flash_is_driver_inited()
{
	return s_flash_client_init;
}


