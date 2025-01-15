/**
 **************************************************************************************
 * @file    bkreg.c
 * @brief   BEKEN registor operation by UART
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include <stdint.h>
#include <string.h>
#include "bkreg.h"
#include "bkstdio.h"
#include "drv_uart.h"
#if defined (STAR_SE)
#include "STAR_SE.h"
#endif

#define BEKEN_OCF                   (0xE0)
#define HCI_EVENT_HEAD_LENGTH       (0x03)
#define HCI_COMMAND_HEAD_LENGTH     (0x04)
#define HCI_COMMAND_COMPLETE_EVENT  (0x0E)
#define VENDOR_SPECIFIC_DEBUG_OGF   (0x3F)

#define __PACKED_POST__             __attribute__((packed))

enum
{
    TRA_HCIT_COMMAND = 1,
    TRA_HCIT_ACLDATA = 2,
    TRA_HCIT_SCODATA = 3,
    TRA_HCIT_EVENT   = 4
};

enum
{
    BEKEN_UART_LINK_CHECK                       = 0x00,  /*return 0x04 0x0e 0x04 0x01 0xe0 0xfc 0x00*/
    BEKEN_UART_REGISTER_WRITE_CMD               = 0x01,  /*Write certain reg*/
    BEKEN_UART_REGISTER_CONTINUOUS_WRITE_CMD    = 0x02,  /*write reg continuous*/
    BEKEN_UART_REGISTER_READ_CMD                = 0x03,  /*read certain reg value*/
    BEKEN_UART_BT_START_CMD                     = 0x04,  /*useless*/
    BEKEN_UART_BT_STOP_CMD                      = 0x05,  /*useless*/
    BEKEN_UART_PATCH_CMD                        = 0x06,  /*useless*/
    BEKEN_UART_SET_UART_PROTOCOL                = 0x07,  /*set uart protocol, H4:00  BCSP:01*/
    BEKEN_DUT_MODE_ENABLE                       = 0xAF,  /* Enter Dut mode */
    BEKEN_UART_SYS_RESET_CMD                    = 0xFE,
};

typedef struct
{
    unsigned char code;             /**< 0x01: HCI Command Packet
                                         0x02: HCI ACL Data Packet
                                         0x03: HCI Synchronous Data Packet
                                         0x04: HCI Event Packet */
    struct
    {
        unsigned short ocf : 10;    /**< OpCode Command Field */
        unsigned short ogf : 6;     /**< OpCode Group Field */
    }__PACKED_POST__ opcode;

    unsigned char total;
    unsigned char param[];

}__PACKED_POST__ HCI_PACKET;

typedef struct
{
    unsigned char code;             /**< 0x01: HCI Command Packet
                                         0x02: HCI ACL Data Packet
                                         0x03: HCI Synchronous Data Packet
                                         0x04: HCI Event Packet */
    struct
    {
        unsigned short ocf : 10;    /**< OpCode Command Field */
        unsigned short ogf : 6;     /**< OpCode Group Field */
    }__PACKED_POST__ opcode;

    unsigned char total;
    unsigned char cmd;              /**< private command */
    unsigned char param[];

}__PACKED_POST__ HCI_COMMAND_PACKET;

typedef struct
{
    unsigned char code;             /**< 0x01: HCI Command Packet
                                         0x02: HCI ACL Data Packet
                                         0x03: HCI Synchronous Data Packet
                                         0x04: HCI Event Packet */
    unsigned char event;            /**< 0x00-0xFF: Each event is assigned a 1-Octet event code used to uniquely identify different types of events*/
    unsigned char total;            /**< Parameter Total Length */
    unsigned char param[];

}__PACKED_POST__ HCI_EVENT_PACKET;

typedef struct
{
    unsigned int addr;
    unsigned int value;
}__PACKED_POST__ REGISTER_PARAM;

unsigned int  REG_XVR_BAK[16];

int bkreg_process(char* buf, int len)
{
    HCI_COMMAND_PACKET* pHCIrxBuf = (HCI_COMMAND_PACKET*)buf;

    if(len == 0) return 0;

    if((pHCIrxBuf->code       == TRA_HCIT_COMMAND)
    && (pHCIrxBuf->opcode.ogf == VENDOR_SPECIFIC_DEBUG_OGF)
    && (pHCIrxBuf->opcode.ocf == BEKEN_OCF)
    && (len                   >= (HCI_COMMAND_HEAD_LENGTH + pHCIrxBuf->total)))
    {
        uint8_t  tx_buf[64];
        uint32_t tx_flag = 1;

        HCI_EVENT_PACKET* pHCItxBuf = (HCI_EVENT_PACKET*)tx_buf;

        //bk_printf("[BKREG]: cmd = %x\r\n", pHCIrxBuf->cmd);

        switch(pHCIrxBuf->cmd)
        {
        case BEKEN_UART_LINK_CHECK:
            pHCItxBuf->total = len;
            memcpy(pHCItxBuf->param, buf, pHCItxBuf->total);
            break;

        case BEKEN_UART_REGISTER_WRITE_CMD:
            {
                REGISTER_PARAM *rx_param = (REGISTER_PARAM *)pHCIrxBuf->param;
                REGISTER_PARAM *tx_param = (REGISTER_PARAM *)&pHCItxBuf->param[HCI_COMMAND_HEAD_LENGTH];

                memcpy(pHCItxBuf->param, buf, HCI_EVENT_HEAD_LENGTH);
                pHCItxBuf->total    = len - 1;
                pHCItxBuf->param[3] = pHCIrxBuf->cmd;

                tx_param->addr  = rx_param->addr;
                tx_param->value = rx_param->value;

                *(volatile unsigned int *)rx_param->addr = rx_param->value;

                int reg_idx = (rx_param->addr - MDU_XVR_BASE_ADDR) / 4;
                if((reg_idx >= 0) && (reg_idx <= 0x0f)) REG_XVR_BAK[reg_idx] = rx_param->value;
            }
            break;

        case BEKEN_UART_REGISTER_READ_CMD:
            {
                REGISTER_PARAM* rx_param = (REGISTER_PARAM *)pHCIrxBuf->param;
                REGISTER_PARAM* tx_param = (REGISTER_PARAM *)&pHCItxBuf->param[HCI_COMMAND_HEAD_LENGTH];

                memcpy(pHCItxBuf->param, buf, HCI_EVENT_HEAD_LENGTH);
                pHCItxBuf->total    = HCI_EVENT_HEAD_LENGTH + len;
                pHCItxBuf->param[3] = pHCIrxBuf->cmd;

                tx_param->addr = rx_param->addr;

                int reg_idx = (rx_param->addr - MDU_XVR_BASE_ADDR) / 4;
                if((reg_idx >= 0) && (reg_idx <= 0x0f))
                {
                    tx_param->value = REG_XVR_BAK[reg_idx];
                }
                else
                {
                    tx_param->value = *(volatile unsigned int *)rx_param->addr;
                }
            }
            break;

        #if 0
        case BEKEN_DUT_MODE_ENABLE:
            {
                void hci_uart_rx_packet(uint8_t* buff, uint32_t size);

                const static uint8_t bluecmdDut0[] = { 0x01, 0x1A, 0x0c, 0x01, 0x03 };
                const static uint8_t bluecmdDut1[] = { 0x01, 0x05, 0x0c, 0x03, 0x02, 0x00, 0x02 };
                const static uint8_t bluecmdDut2[] = { 0x01, 0x03, 0x18, 0x00 };

                hci_uart_rx_packet((uint8_t*)bluecmdDut0, sizeof(bluecmdDut0));
                hci_uart_rx_packet((uint8_t*)bluecmdDut1, sizeof(bluecmdDut1));
                hci_uart_rx_packet((uint8_t*)bluecmdDut2, sizeof(bluecmdDut2));

                tx_flag = 0;
            }
            break;
        #endif

        case BEKEN_UART_SYS_RESET_CMD:
            {
                tx_flag = 0;

                /* watchdog reset for uart download */
                if(pHCIrxBuf->total >= 5)
                {
                    uint32_t param = pHCIrxBuf->param[0]<<24 | pHCIrxBuf->param[1]<<16 | pHCIrxBuf->param[2]<<8 | pHCIrxBuf->param[3];
                    if(param == 0x95279527)
                    {
                        /* watchdog reset */
                        REG_WDT_CONFIG = 0x5A0001;
                        REG_WDT_CONFIG = 0xA50001;
                        REG_AONWDT_CONFIG = 0x5A0001;
                        REG_AONWDT_CONFIG = 0xA50001;
                        REG_SYSTEM_0x02 = 1;
                        NVIC_SystemReset();
                    }
                }
            }
            break;

        default:
            tx_flag = 0;
            break;
        }

        if(tx_flag)
        {
            unsigned int tx_len = HCI_EVENT_HEAD_LENGTH + pHCItxBuf->total;
            pHCItxBuf->code  = TRA_HCIT_EVENT;
            pHCItxBuf->event = HCI_COMMAND_COMPLETE_EVENT;
            uart_send(PRINT_UART_PORT, (uint8_t*)pHCItxBuf, tx_len);
        }

        return 1;
    }

    return 0;
}
