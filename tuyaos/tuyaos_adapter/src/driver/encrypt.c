/**
 * @file   encrypt.c
 * @author nRzo <nrzo@nrzo-laptop>
 * @date   Mon Dec 12 21:16:14 2011
 *
 * @brief
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned int   BOOL;


#define ENCRYPT_SIZE      0X04
#define CRC_SIZE          0X02
#define CRC_PAKEIT        0X20

uint32_t  coef0 = 0x510fb093;//0x12345678;
uint32_t  coef1 = 0xa3cbeadc;//0x2faa55aa;
uint32_t  coef2 = 0x5993a17e;//0x3aee63dd;
uint32_t  coef3 = 0xc7adeb03;//0x4feeaa00;

typedef  struct __attribute__((packed)) encrypt_packet 
{
    u8 data[CRC_PAKEIT];
    
    struct __attribute__((packed)) 
    {
        u16 bit8  : 1;
        u16 bit9  : 1;
        u16 bit10 : 1;
        u16 bit11 : 1;
        u16 bit12 : 1;
        u16 bit13 : 1;
        u16 bit14 : 1;
        u16 bit15 : 1;
        
        u16 bit0  : 1;
        u16 bit1  : 1;
        u16 bit2  : 1;
        u16 bit3  : 1;
        u16 bit4  : 1;
        u16 bit5  : 1;
        u16 bit6  : 1;
        u16 bit7  : 1;
    }crc;
}ENCRYPT_PACKET_T;
/*
	pn15_gen u_pn15(.pn_out(pn15),.addr(pn15_addr),.rstn(rstn),.clk(clk),.bps(pn15_bps));
*/
u16 pn15_gen_u_pn15(u16 addr, u8 bps)
{
    u16 pn_out;
    if(bps)
        pn_out = 0;
    else
    {
        /*
        	wire [15:0]   pn_tmp = {addr[6:0], addr[15:7]} ^ {16'h6371 & {4{addr[8:5]}}};
        */
        u16 a, b;
        a = ((addr & 0x7f) << 9) + ((addr >> 7) & 0x1ff);
        b = 0x6371 & ( (((addr >> 5) & 0xf) << 12) + (((addr >> 5) & 0xf) << 8)
                       + (((addr >> 5) & 0xf) << 4) + (((addr >> 5) & 0xf) << 0) );
        pn_out = a ^ b;
    }
    return pn_out;
}

/*
	pn16_gen u_pn16(.pn_out(pn16),.addr(pn16_addr),.rstn(rstn),.clk(clk),.bps(pn16_bps));
*/
u32 pn16_gen_u_pn16(u32 addr, u8 bps)
{
    u32 pn_out;
    if(bps)
        pn_out = 0;
    else
    {
        /*
        	wire [16:0]   pn_tmp = {addr[9:0], addr[16:10]} ^ {17'h13659 & {addr[4],{4{addr[1],addr[5],addr[9],addr[13]}}}};
        */
        u32 a, b, c;
        a = ((addr & 0x3ff) << 7) + ((addr >> 10) & 0x7f);
        b = (((addr >> 1) & 0x1) << 3) + (((addr >> 5) & 0x1) << 2) + (((addr >> 9) & 0x1) << 1) + (((addr >> 13) & 0x1) << 0);
        c = 0x13659 & ( (((addr >> 4) & 0x1) << 16) + (b << 12) + (b << 8) + (b << 4) + b );
        pn_out = a ^ c;
    }
    return pn_out;
}

/*
	pn32_gen u_pn32(.pn_out(pn32),.addr(pn32_addr),.rstn(rstn),.clk(clk),.bps(pn32_bps));
*/
u32 pn32_gen_u_pn32(u32 addr, u8 bps)
{
    u32 pn_out;
    if(bps)
        pn_out = 0;
    else
    {
        /*
        	wire [31:0]   pn_tmp = {addr[14:0], addr[31:15]} ^ {32'hE519A4F1 & {8{addr[5:2]}}};
        */
        u32 a, b, c;
        a = ((addr & 0x7fff) << 17) + ((addr >> 15) & 0x1ffff);
        b = ((addr >> 2) & 0xf);
        c = 0xE519A4F1 & ( (b << 28) + (b << 24) + (b << 20) + (b << 16) + (b << 12) + (b << 8) + (b << 4) + b );
        pn_out = a ^ c;
    }
    return pn_out;
}


u32 enc_data_my(u32 addr, u32 raw_data)
{
    u8 g_bypass;
    u8 pn15_bps;
    u8 pn16_bps;
    u8 pn32_bps;
    u8 rand_bps;

    u8 pn16_bit;
    u8 pn15_sel;
    u8 pn16_sel;
    u8 pn32_sel;

    u16 pn15_A;
    u32 pn16_A;
    u32 pn32_A;

    u16 pn15_addr;
    u32 pn16_addr;
    u32 pn32_addr;

    u16 pn15;
    u16 pn16;
    u32 pn32;
    u32 random;
    u32 pnout;
    u32 enc_dat;
    
    //printf("raw_data: 0x%x ", raw_data);
    
    /*
    	wire      g_bypass = (coef3[31:24]==8'hFF) |
                         	(coef3[31:24]==8'h00) ;
    */
    enc_dat = coef3 & 0xff000000;
    if( (enc_dat == 0xff000000) || (enc_dat == 0x0) )
        g_bypass = 1;
    else
        g_bypass = 0;
    /*
    	wire      pn15_bps = g_bypass | coef3[0] ;
    	wire      pn16_bps = g_bypass | coef3[1] ;
    	wire      pn32_bps = g_bypass | coef3[2] ;
    	wire      rand_bps = g_bypass | coef3[3] ;
    */
    if( g_bypass || (coef3 & 0x1))
        pn15_bps = 1;
    else
        pn15_bps = 0;

    if( g_bypass || (coef3 & 0x2))
        pn16_bps = 1;
    else
        pn16_bps = 0;

    if( g_bypass || (coef3 & 0x4))
        pn32_bps = 1;
    else
        pn32_bps = 0;

    if( g_bypass || (coef3 & 0x8))
        rand_bps = 1;
    else
        rand_bps = 0;
    /*
    	wire      pn16_bit = coef3[    4] ;
    	wire[1:0] pn15_sel = coef3[ 6: 5] ;
    	wire[1:0] pn16_sel = coef3[ 9: 8] ;
    	wire[1:0] pn32_sel = coef3[12:11] ;
    */
    pn16_bit = (coef3 >> 4) & 0x1;
    pn15_sel = (coef3 >> 5) & 0x3;
    pn16_sel = (coef3 >> 8) & 0x3;
    pn32_sel = (coef3 >> 11) & 0x3;

    /*
    	wire[15:0] pn15_A  = (pn15_sel==0) ? ({addr[31:24],addr[23:16]} ^ {addr[15:8],addr[7:0]}) :
    	                     (pn15_sel==1) ? ({addr[31:24],addr[23:16]} ^ {addr[7:0],addr[15:8]}) :
    	                     (pn15_sel==2) ? ({addr[23:16],addr[31:24]} ^ {addr[15:8],addr[7:0]}) :
    	                                     ({addr[23:16],addr[31:24]} ^ {addr[7:0],addr[15:8]}) ;
    */
    if(pn15_sel == 0)
    {
        u16 a, b;
        a = (((addr >> 24) & 0xff) << 8) + ((addr >> 16) & 0xff);
        b = (((addr >> 8 ) & 0xff) << 8) + ((addr >> 0 ) & 0xff);
        pn15_A = a ^ b;
    }
    else if(pn15_sel == 1)
    {
        u16 a, b;
        a = (((addr >> 24) & 0xff) << 8) + ((addr >> 16) & 0xff);
        b = (((addr >> 0 ) & 0xff) << 8) + ((addr >> 8 ) & 0xff);
        pn15_A = a ^ b;
    }
    else if(pn15_sel == 2)
    {
        u16 a, b;
        a = (((addr >> 16) & 0xff) << 8) + ((addr >> 24) & 0xff);
        b = (((addr >> 8 ) & 0xff) << 8) + ((addr >> 0 ) & 0xff);
        pn15_A = a ^ b;
    }
    else
    {
        u16 a, b;
        a = (((addr >> 16) & 0xff) << 8) + ((addr >> 24) & 0xff);
        b = (((addr >> 0 ) & 0xff) << 8) + ((addr >> 8 ) & 0xff);
        pn15_A = a ^ b;
    }

    /*
    	wire[16:0] pn16_A  = (pn16_sel==0) ?   addr[16:0] :
    	                     (pn16_sel==1) ?   addr[17:1] :
    	                     (pn16_sel==2) ?   addr[18:2] :
    	                                       addr[19:3] ;
    */
    if(pn16_sel == 0)
        pn16_A = (addr >> 0) & 0x1ffff;
    else if(pn16_sel == 1)
        pn16_A = (addr >> 1) & 0x1ffff;
    else if(pn16_sel == 2)
        pn16_A = (addr >> 2) & 0x1ffff;
    else
        pn16_A = (addr >> 3) & 0x1ffff;

    /*
    	wire[31:0] pn32_A  = (pn32_sel==0) ?   addr[31:0] :
    	                     (pn32_sel==1) ?  {addr[ 7:0],addr[31: 8]} :
    	                     (pn32_sel==2) ?  {addr[15:0],addr[31:16]} :
    	                                      {addr[23:0],addr[31:24]} ;
    */
    if(pn32_sel == 0)
        pn32_A = addr;
    else if(pn32_sel == 1)
        pn32_A = (((addr >> 0) & 0xff    ) << 24) + ((addr >> 8 ) & 0xffffff);
    else if(pn32_sel == 2)
        pn32_A = (((addr >> 0) & 0xffff  ) << 16) + ((addr >> 16) & 0xffff);
    else
        pn32_A = (((addr >> 0) & 0xffffff) << 8 ) + ((addr >> 24) & 0xff);


    /*
    	wire[15:0] pn15_addr = pn15_A ^ coef1[31:16] ;
    	wire[16:0] pn16_addr = pn16_A ^ {coef1[15:8], pn16_bit, coef1[7:0]} ;
    	wire[31:0] pn32_addr = pn32_A ^ coef0[31:0] ;
    */
    pn15_addr = pn15_A ^ ((coef1 >> 16) & 0xffff);
    pn16_addr = (pn16_A ^ (((coef1 >> 8) & 0xff) << 9)) + (pn16_bit << 8) + (coef1 & 0xff);
    pn32_addr = pn32_A ^ coef0;

    /*
    	pn15_gen u_pn15(.pn_out(pn15),.addr(pn15_addr),.rstn(rstn),.clk(clk),.bps(pn15_bps));
    	pn16_gen u_pn16(.pn_out(pn16),.addr(pn16_addr),.rstn(rstn),.clk(clk),.bps(pn16_bps));
    	pn32_gen u_pn32(.pn_out(pn32),.addr(pn32_addr),.rstn(rstn),.clk(clk),.bps(pn32_bps));
    */
    pn15 = pn15_gen_u_pn15(pn15_addr, pn15_bps);
    pn16 = pn16_gen_u_pn16(pn16_addr, pn16_bps);
    pn32 = pn32_gen_u_pn32(pn32_addr, pn32_bps);

    /*
    	wire[31:0] random = rand_bps ? 32'h00000000 : coef2[31:0] ;
    */
    if(rand_bps == 1)
        random = 0;
    else
        random = coef2;
    /*
    	assign     pnout  = pn32[31:0] ^ {pn15[15:0],pn16[15:0]} ^ random[31:0] ;
    */

    pnout = pn32 ^ ((pn15 << 16) + (pn16 & 0xffff)) ^ random;

    enc_dat  = raw_data ^ pnout;
    
    return enc_dat;
}

void ty_calc_crc(u32 *buf, u32 packet_num)
{
    int i, bit, byte;

    for (i = 0; i < (int)packet_num; i++)
    {
        ENCRYPT_PACKET_T *p = (struct encrypt_packet *)((u32)buf + i * sizeof(ENCRYPT_PACKET_T));
        ENCRYPT_PACKET_T bak;
		
        for (byte = 0; byte < CRC_PAKEIT; byte++)
        {
            for (bit = 7; bit >= 0; bit--)
            {
                u8 input;
				
                memcpy((char *)&bak, (char *)p, sizeof(ENCRYPT_PACKET_T));
				
                input = (bak.data[byte] >> bit) & 0x01;
                p->crc.bit15 =  bak.crc.bit14 ^ input ^ bak.crc.bit15;
                p->crc.bit14 =  bak.crc.bit13;
                p->crc.bit13 =  bak.crc.bit12;
                p->crc.bit12 =  bak.crc.bit11;
                p->crc.bit11 =  bak.crc.bit10;
                p->crc.bit10 =  bak.crc.bit9;
                p->crc.bit9  =  bak.crc.bit8;
                p->crc.bit8  =  bak.crc.bit7;
                p->crc.bit7  =  bak.crc.bit6;
                p->crc.bit6  =  bak.crc.bit5;
                p->crc.bit5  =  bak.crc.bit4;
                p->crc.bit4  =  bak.crc.bit3;
                p->crc.bit3  =  bak.crc.bit2;
                p->crc.bit2  =  bak.crc.bit1 ^ input ^ bak.crc.bit15;
                p->crc.bit1  =  bak.crc.bit0;
                p->crc.bit0  =  input ^ bak.crc.bit15;
            }												
        }
    }
}

void ty_get_calc_buf(u8 *rx, u8 *tx, u32 num)
{ 
    u32 i, j, rx_index = 0, tx_index = 0; 

    for (i = 0; i < num; i++)
    {
        for (j = 0; j < CRC_PAKEIT; j++)
        {
            tx[tx_index] = rx[rx_index];
            tx_index++;
            rx_index++;
        }

        tx[tx_index + 0] = 0xff;
        tx[tx_index + 1] = 0xff;
        tx_index += 2;

    }

}
    
void ty_encrypt(u32 *rx, u8 *tx, u32 num,u32  addr0)
{
    u8 *tmp_rx;
    u32 rx_value, ret;
    u32 i, j, addr, addr1;
    u32 len = CRC_PAKEIT / sizeof(u32);

    addr0 = (addr0 * 32)/34;
    
    for (i = 0; i < num; i++)
    {
        for (j = 0; j < len; j++)
        {
            addr = i * (CRC_PAKEIT + sizeof(u16)) + j * sizeof(u32);
            addr1 = addr0 + i * (CRC_PAKEIT) + j * sizeof(u32);

            tmp_rx = (u8 *)rx;
            rx_value = (tmp_rx[0] << (0 * 8)) + (tmp_rx[1] << (1 * 8))
                                                + (tmp_rx[2] << (2 * 8))
                                                + (tmp_rx[3] << (3 * 8));
            ret = enc_data_my(addr1, rx_value);
            tx[addr + 0] = (ret >> (0 * 8)) & 0xff;
            tx[addr + 1] = (ret >> (1 * 8)) & 0xff;
            tx[addr + 2] = (ret >> (2 * 8)) & 0xff;
            tx[addr + 3] = (ret >> (3 * 8)) & 0xff;
            
            rx ++;
        }

        tx[addr + sizeof(u32) + 0] = 0xff;
        tx[addr + sizeof(u32) + 1] = 0xff;
    }
}

