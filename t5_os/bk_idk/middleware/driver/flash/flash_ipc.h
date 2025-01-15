#ifndef _FLASH_IPC_H_
#define _FLASH_IPC_H_

#include <common/bk_include.h>

enum
{
	FLASH_CMD_ERASE_SECTOR = 0,
	FLASH_CMD_ERASE_32k,
	FLASH_CMD_ERASE_BLOCK,
	FLASH_CMD_READ,
	FLASH_CMD_READ_DONE,
	FLASH_CMD_WRITE,
} ;

#define FLASH_OPERATE_TIMEOUT         600

typedef struct
{
	u32     part_id  : 8;
	u32     addr     : 24;
	u8    * buff;
	u16     len;
	int16   ret_status;
	u32     crc;
} flash_cmd_t;

#define FLASH_IPC_READ_SIZE     0x200
#define FLASH_IPC_WRITE_SIZE    0x200

#endif //_FLASH_IPC_H_
// eof

