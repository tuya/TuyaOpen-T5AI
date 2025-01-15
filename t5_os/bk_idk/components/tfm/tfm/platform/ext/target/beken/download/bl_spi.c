/*
 * bl_spi.c
 *
 *  Created on: 2017-4-27
 *      Author: gang.cheng
 */

#include <stdbool.h>          // standard boolean definitions
#include <stdint.h>           // standard integer functions

#include "type.h"
#include "bl_spi.h"
#include "bl_uart.h"
#include "system.h"
#include <string.h>
#include "bl_bk_hci_protocol.h"

///SPI Character format
enum SPI_CHARFORMAT
{
    SPI_CHARFORMAT_8BIT = 0x00UL,
    SPI_CHARFORMAT_16BIT = 0x01UL
};


enum SPI_EN
{
	 SPI_DISABLE = 0x00UL,
	 SPI_ENABLE = 0x01UL
};

enum CKPHA
{
	 CKPHA_CLK1 = 0x00UL,
	 CKPHA_CLK2 = 0x01UL
};

enum CKPOL
{
	 CKPOL_L = 0x00UL,
	 CKPOL_H = 0x01UL
};

enum NSSMD
{
	LINE3_NSS_NULL = 0,
	LINE4_NSS_CHIP = 1,
	LINE4_NSS_M = 2,
	LINE4_NSS_S_E =  3,

};


void GPIO_SPI_function_enable(void)
{

    // Enable GPIO 6,7,8,9 peripheral function for SPI

		BK3000_GPIO_6_CONFIG = ((0x01 << sft_GPIO_FUNCTION_ENABLE) | (0x07 << 3));


        BK3000_GPIO_7_CONFIG = ((0x01 << sft_GPIO_FUNCTION_ENABLE)  | (0x07 << 3));


        BK3000_GPIO_8_CONFIG = ((0x01 << sft_GPIO_FUNCTION_ENABLE) | (0x07 << 3));

        BK3000_GPIO_9_CONFIG = ((0x01 << sft_GPIO_FUNCTION_ENABLE)  | (0x07 << 3));


}





void spi_int_enable(void)
{
#if (SPI_INT_MODE)
    intc_module_enable_set(INT_SPI_bit);             // SPI interrupt enable
#endif
}


void spi_initial(SPI_ROLE role,uint32_t baud)
{
#if (BK3266_ROM_DEBUG)
	 GPIO_SPI_function_enable();
#endif
	REG_SPI_RST = 3;
	 if(SPI_MASTER == role)
	 {
			REG_SPI_CTRL = (SPI_ENABLE           << BIT_SPI_CTRL_SPIEN)
													|( role << BIT_SPI_CTRL_MSTEN)
													| (CKPHA_CLK1           << BIT_SPI_CTRL_CKPHA)
													| (CKPOL_L           << BIT_SPI_CTRL_CKPOL)
													| (SPI_CHARFORMAT_8BIT           << BIT_SPI_CTRL_BIT_WIDTH)
													| (0x02UL           << BIT_SPI_CTRL_NSSMD)
												//	| (SPI_CLK_DIVID_SET(baud)  << BIT_SPI_CTRL_SPI_CKR)
													|( 1  << BIT_SPI_CTRL_SPI_CKR)
													| (0x00UL           << BIT_SPI_CTRL_RXINT_EN)
													| (0x00UL           << BIT_SPI_CTRL_TXINT_EN)
													| (0x00UL           << BIT_SPI_CTRL_RXOVR_EN)
													| (0x00UL           << BIT_SPI_CTRL_TXOVR_EN)
													| (0x00UL           << BIT_SPI_CTRL_RXINT_MODE)
													| (0x00UL           << BIT_SPI_CTRL_TXINT_MODE);
	 }
	 else if(SPI_SLAVE == role)
	 {
			REG_SPI_CTRL = (0x01UL           << BIT_SPI_CTRL_SPIEN)
													|( role							 << BIT_SPI_CTRL_MSTEN)
													| (CKPHA_CLK1           << BIT_SPI_CTRL_CKPHA)
													| (CKPOL_L           << BIT_SPI_CTRL_CKPOL)
													| (SPI_CHARFORMAT_8BIT           << BIT_SPI_CTRL_BIT_WIDTH)
													| (0x02UL           << BIT_SPI_CTRL_NSSMD)
													| (SPI_CLK_DIVID_SET(baud)  << BIT_SPI_CTRL_SPI_CKR)
													| (0x00UL           << BIT_SPI_CTRL_RXINT_EN)
													| (0x01UL           << BIT_SPI_CTRL_TXINT_EN)
													| (0x01UL           << BIT_SPI_CTRL_RXOVR_EN)
													| (0x01UL           << BIT_SPI_CTRL_TXOVR_EN)
													| (0x00UL           << BIT_SPI_CTRL_RXINT_MODE)
													| (0x00UL           << BIT_SPI_CTRL_TXINT_MODE);

	 }

	 spi_int_enable();

}

void spi_tx_start(void)
{
		uint32_t reg ;
	//	bl_printf("spi_tx_start\r\n");
	                                               // reset css
			reg = REG_SPI_CTRL;                        // sel wire4
  		reg &= ~(0x1 << BIT_SPI_CTRL_WIRE3_EN) ;
			REG_SPI_CTRL = reg; 
	
		reg = REG_SPI_CN;                            //enable tx_en rx_ten
	  reg |= 0x03UL; 
	  REG_SPI_CN = reg ;
	
}



void spi_tx_end(void)
{
		uint32_t reg ;
		uint32_t state;
		reg  = REG_SPI_CTRL;

		reg |=(0x01UL << BIT_SPI_CTRL_TXINT_EN);
		REG_SPI_CTRL = reg;
	
#if (!SPI_INT_MODE)
		state = REG_SPI_STAT;
		while(!(state & (0x01 << BIT_SPI_STAT_TXINT)))
		{
			state = REG_SPI_STAT;
		}
		reg &=~(0x01UL << BIT_SPI_CTRL_TXINT_EN);
		reg |= (0x1 << BIT_SPI_CTRL_WIRE3_EN);      // reset nss
		REG_SPI_CTRL = reg;
		REG_SPI_STAT = state;

		//reg |= (0x03UL << BIT_SPI_CTRL_NSSMD);
		//REG_SPI_CTRL = reg;
		REG_SPI_CN = 0;
		
#endif
	//	bl_printf("spi_tx_end\r\n");
}




uint8_t spi_writeByte_for_4k(uint8_t *data,uint8_t len)
{

	   uint8_t w_cnt = 0;

		while(/*(!spi_is_busy()) && */(!SPI_TX_FIFO_FULL)&& (len--))
		{
			REG_SPI_DAT = *data++;
			w_cnt++;
		}

		return w_cnt;

}



uint8_t spi_writeByte1(uint8_t *data,uint8_t len)
{
	uint8_t status = STATUS_OK;

	set_1mstime_cnt(0);
	while(len--)
	{
		while(spi_is_busy() || SPI_TX_FIFO_FULL )
			{
				if(get_1mstime_cnt() > 50 )
				{

					status = SPI_OP_T_OUT;
					return status;
				}
			}
				REG_SPI_DAT = *data++;

	}
	return status;
//	return w_cnt;
}

uint8_t spi_readByte_for_4k(u8 *data,u8 len)
{

	uint8_t r_cnt = 0;

	while(/*spi_is_busy() || */ (!SPI_RX_FIFO_EMPTY) && (len--))
	{

		*data++= REG_SPI_DAT & 0xff;
		r_cnt++;
		//if(len == 2)bl_printf("SPI_RX_FIFO_EMPTY2 = %x\r\n",SPI_RX_FIFO_EMPTY);
	//	if(len == 5)bl_printf("SPI_RX_FIFO_EMPTY5 = %x\r\n",SPI_RX_FIFO_EMPTY);
	}



	return r_cnt;

}

uint8_t spi_readByte1(uint8_t *data,uint8_t len)
{

	uint8_t status = STATUS_OK;


	set_1mstime_cnt(0);

	while(len--)
	{
		while(/*spi_is_busy() || */SPI_RX_FIFO_EMPTY )
		{
			if(get_1mstime_cnt() > 50 )
			{

				status = SPI_OP_T_OUT;
				return status;
			}

		}
		*data++= REG_SPI_DAT & 0xff;
	}

	return status;

}



#define  SPI_FIFO_LEN  0X10



uint8_t spi_write_read_test()
{

	uint8_t empty_rxbuf[SPI_FIFO_LEN] = {0};
	return spi_writeByte1(empty_rxbuf,SPI_FIFO_LEN);
//	r_cnt = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN);
//	bl_printf("r_cnt = %d\r\n",r_cnt);

}

uint8_t spi_write_read_clean()
{
	uint8_t data;
//	bl_printf("REG_SPI_CTRL = %x\r\n",REG_SPI_CTRL);
	REG_SPI_CTRL |= (0x01 << BIT_SPI_CTRL_RXFIFO_CLR);
	REG_SPI_CTRL &= ~(0x01 << BIT_SPI_CTRL_RXFIFO_CLR);
//	set_1mstime_cnt(0);

	while(!SPI_RX_FIFO_EMPTY )
	{
		data = REG_SPI_DAT & 0xff;
		data = data;
	//	bl_printf("data = %x\r\n",data);
	}

	return STATUS_OK;
}

uint8_t spi_write_read_4K_R(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset)
{

		uint32_t max_len;

		uint32_t w_total;
		uint32_t r_total;

		uint32_t w_cnt;
		uint32_t r_cnt;
		uint32_t r_cnt_temp;


		uint8_t status = STATUS_OK;



		uint8_t empty_txbuf[SPI_FIFO_LEN] = {0};

		uint8_t empty_rxbuf[SPI_FIFO_LEN] = {0};


	//	bl_printf("%s\r\n",__func__);

		memset(empty_txbuf,0,SPI_FIFO_LEN);

		max_len = (w_size > (r_size + r_offset)) ? w_size : (r_size + r_offset);
		w_total = max_len;
		r_total  = max_len;
		spi_tx_start();
		w_cnt = 0;
		r_cnt = 0;
		set_5mstime_cnt(0);
		while(max_len > 0)
		{
			if(w_size > SPI_FIFO_LEN)
			{
				w_cnt = spi_writeByte_for_4k(wbuf,SPI_FIFO_LEN);
				wbuf +=w_cnt ;
				w_size -= w_cnt;
				w_total-= w_cnt;
			}else if(w_size == 0)
			{
				if(w_total > 0)
				{

					while((!SPI_TX_FIFO_EMPTY))
					{
						;
					}
					w_total = 0;

					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;

					w_total = 0;
				}

			}
			else
			{
				w_cnt = spi_writeByte_for_4k(wbuf,w_size);
			//	bl_printf("w_total1 = %d,w_size = %d,w_cnt = %d\r\n",w_total,w_size,w_cnt);
				wbuf +=w_cnt ;
				w_size-=w_cnt;
				w_total-= w_cnt;

			}


			if(r_offset > SPI_FIFO_LEN)
			{

				r_cnt = spi_readByte_for_4k(empty_rxbuf,SPI_FIFO_LEN);
			//	r_cnt = SPI_FIFO_LEN;
			//	spi_write_read_clean();
			//	bl_printf("r_total1 = %d,r_cnt = %d\r\n",r_total,r_cnt);
				r_offset-= r_cnt;
				r_total -=r_cnt;
			}else if (r_offset > 0)
			{
				r_cnt = spi_readByte_for_4k(empty_rxbuf,r_offset);
			//	r_cnt = r_offset;
			//	spi_write_read_clean();
		//		bl_printf("r_total2 = %d,r_cnt = %d\r\n",r_total,r_cnt);
				r_offset -= r_cnt;
				r_total -=r_cnt;
			}
			else if(r_size > SPI_FIFO_LEN)
			{
				r_cnt = spi_readByte_for_4k(rbuf,SPI_FIFO_LEN);
				r_cnt_temp = r_cnt;
				rbuf+= r_cnt;
			/*	while((!SPI_RX_FIFO_EMPTY))
				{

					*rbuf++= REG_SPI_DAT_CHAR ;
					r_cnt++;
							//if(len == 2)bl_printf("SPI_RX_FIFO_EMPTY2 = %x\r\n",SPI_RX_FIFO_EMPTY);
						//	if(len == 5)bl_printf("SPI_RX_FIFO_EMPTY5 = %x\r\n",SPI_RX_FIFO_EMPTY);
				}
				*/
				r_size -= r_cnt;
				r_total -=r_cnt;
				while(r_cnt_temp--)
					{

						REG_SPI_DAT_CHAR = 0 ;

											//if(len == 2)bl_printf("SPI_RX_FIFO_EMPTY2 = %x\r\n",SPI_RX_FIFO_EMPTY);
										//	if(len == 5)bl_printf("SPI_RX_FIFO_EMPTY5 = %x\r\n",SPI_RX_FIFO_EMPTY);
						}
			//	r_cnt = SPI_FIFO_LEN;
			//	for(i = 0;i < r_cnt;i++)
			//	{
			//		bl_printf("rbuf0[%d] = %x\r\n",i,rbuf[i]);
			//	}
			//	bl_printf("r_total3 = %d,r_cnt = %d\r\n",r_total,r_cnt);


			}
			else if(r_size > 0)
			{
					r_cnt = spi_readByte_for_4k(rbuf,r_size);

					r_cnt_temp = r_cnt;
				//	bl_printf("r_total4 = %d,r_size = %d,r_cnt = %d\r\n",r_total,r_size,r_cnt);


					r_total -=r_cnt;
					if(r_size > 8)
					{
						r_cnt_temp = r_size - 8;
						while(r_cnt_temp--)
						{

							REG_SPI_DAT_CHAR = 0 ;

																					//if(len == 2)bl_printf("SPI_RX_FIFO_EMPTY2 = %x\r\n",SPI_RX_FIFO_EMPTY);
																				//	if(len == 5)bl_printf("SPI_RX_FIFO_EMPTY5 = %x\r\n",SPI_RX_FIFO_EMPTY);
						}
					}
					r_size -=r_cnt;
				//	for(i = 0;i < r_cnt;i++)
				//	{
				//		bl_printf("rbuf1[%d] = %x\r\n",i,rbuf[i]);
				//	}
					rbuf+= r_cnt;
			}else
			{

				if(r_total > 0)
				{
				//	bl_printf("r_total5 = %d,r_cnt = %d\r\n",r_total,r_cnt);
					r_cnt = (r_total >= 16) ? SPI_FIFO_LEN : r_total;


				//	spi_write_read_clean();
				//	r_cnt = spi_readByte1(empty_rxbuf,r_cnt);
				//	spi_write_read_clean();
					r_total -=r_cnt;
				//	bl_printf("r_total6 = %d,r_cnt = %d\r\n",r_total,r_cnt);
				}

			}

			//spi_write_read_clean();
			max_len -= r_cnt;
			if(get_5mstime_cnt() > 2000)
			{
				status = FLASH_OP_T_OUT;
				goto ret;
			}
		}

	//	bl_printf("w_total8 = %d,r_total8 = %d\r\n",w_total,r_total);





ret:

		spi_tx_end();

		spi_write_read_clean();

		return status;
}


extern unsigned int crc32_table[256];
extern unsigned int g_crc;
uint8_t spi_write_read_4K_R_for_crc(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset)
{

		uint32_t max_len;

		uint32_t w_total;
		uint32_t r_total;

		uint32_t w_cnt;
		uint32_t r_cnt;
		uint32_t r_cnt_temp;

		uint8_t status = STATUS_OK;



		uint8_t empty_txbuf[SPI_FIFO_LEN] = {0};

		uint8_t empty_rxbuf[SPI_FIFO_LEN] = {0};




	//	bl_printf("%s\r\n",__func__);
		memset(empty_txbuf,0,SPI_FIFO_LEN);

		max_len = (w_size > (r_size + r_offset)) ? w_size : (r_size + r_offset);
		w_total = max_len;
		r_total  = max_len;
		spi_tx_start();
		w_cnt = 0;
		r_cnt = 0;
		set_5mstime_cnt(0);
		while(max_len > 0)
		{
			if(w_size > SPI_FIFO_LEN)
			{
				w_cnt = spi_writeByte_for_4k(wbuf,SPI_FIFO_LEN);
				wbuf +=w_cnt ;
				w_size -= w_cnt;
				w_total-= w_cnt;
			}else if(w_size == 0)
			{
				if(w_total > 0)
				{
					while((!SPI_TX_FIFO_EMPTY))
					{
						;
					}
					w_total = 0;

					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;
					REG_SPI_DAT = 0;

			//		bl_printf("w_total2 = %d,w_cnt = %d\r\n",w_total,w_cnt);
				//	w_total -=w_cnt;
					w_total = 0;
				}

			}
			else
			{
				w_cnt = spi_writeByte_for_4k(wbuf,w_size);
			//	bl_printf("w_total1 = %d,w_size = %d,w_cnt = %d\r\n",w_total,w_size,w_cnt);
				wbuf +=w_cnt ;
				w_size-=w_cnt;
				w_total-= w_cnt;

			}


			if(r_offset > SPI_FIFO_LEN)
			{

				r_cnt = spi_readByte_for_4k(empty_rxbuf,SPI_FIFO_LEN);
			//	r_cnt = SPI_FIFO_LEN;
			//	spi_write_read_clean();
			//	bl_printf("r_total1 = %d,r_cnt = %d\r\n",r_total,r_cnt);
				r_offset-= r_cnt;
				r_total -=r_cnt;
			}else if (r_offset > 0)
			{
				r_cnt = spi_readByte_for_4k(empty_rxbuf,r_offset);
			//	r_cnt = r_offset;
			//	spi_write_read_clean();
		//		bl_printf("r_total2 = %d,r_cnt = %d\r\n",r_total,r_cnt);
				r_offset -= r_cnt;
				r_total -=r_cnt;
			}
			else if(r_size > SPI_FIFO_LEN)
			{
				r_cnt = spi_readByte_for_4k(rbuf,SPI_FIFO_LEN);
				r_cnt_temp = r_cnt;

			//	rbuf+= r_cnt;

			/*	while((!SPI_RX_FIFO_EMPTY))
				{

					*rbuf++= REG_SPI_DAT_CHAR ;
					r_cnt++;
							//if(len == 2)bl_printf("SPI_RX_FIFO_EMPTY2 = %x\r\n",SPI_RX_FIFO_EMPTY);
						//	if(len == 5)bl_printf("SPI_RX_FIFO_EMPTY5 = %x\r\n",SPI_RX_FIFO_EMPTY);
				}
				*/
				r_size -= r_cnt;
				r_total -=r_cnt;
				while(r_cnt_temp--)
					{

					  g_crc = ((g_crc >> 8)^(crc32_table[(g_crc^*rbuf++)&0xff]));
					  //
					//  bl_printf("g_crc0 = %08x \r\n",g_crc);

					  REG_SPI_DAT_CHAR = 0 ;

											//if(len == 2)bl_printf("SPI_RX_FIFO_EMPTY2 = %x\r\n",SPI_RX_FIFO_EMPTY);
										//	if(len == 5)bl_printf("SPI_RX_FIFO_EMPTY5 = %x\r\n",SPI_RX_FIFO_EMPTY);
						}

			//	r_cnt = SPI_FIFO_LEN;
			//	for(i = 0;i < r_cnt;i++)
			//	{
			//		bl_printf("rbuf0[%d] = %x\r\n",i,rbuf[i]);
			//	}
			//	bl_printf("r_total3 = %d,r_cnt = %d\r\n",r_total,r_cnt);


			}
			else if(r_size > 0)
			{
					r_cnt = spi_readByte_for_4k(rbuf,r_size);

					r_cnt_temp = r_cnt;
				//	bl_printf("r_total4 = %d,r_size = %d,r_cnt = %d\r\n",r_total,r_size,r_cnt);

					while(r_cnt_temp--)
					{
						 g_crc = (g_crc >> 8)^(crc32_table[(g_crc^(*rbuf++))&0xff]);
					//	 bl_printf("g_crc1 = %08x \r\n",g_crc);
					}


					r_total -=r_cnt;
					if(r_size >= 16)
					{
						r_cnt_temp = r_size - 16;
						while(r_cnt_temp--)
						{

							REG_SPI_DAT_CHAR = 0 ;

																					//if(len == 2)bl_printf("SPI_RX_FIFO_EMPTY2 = %x\r\n",SPI_RX_FIFO_EMPTY);
																				//	if(len == 5)bl_printf("SPI_RX_FIFO_EMPTY5 = %x\r\n",SPI_RX_FIFO_EMPTY);
						}
					}
					r_size -=r_cnt;


			}else
			{

				if(r_total > 0)
				{
				//	bl_printf("r_total5 = %d,r_cnt = %d\r\n",r_total,r_cnt);
					r_cnt = (r_total >= 16) ? SPI_FIFO_LEN : r_total;


				//	spi_write_read_clean();
				//	r_cnt = spi_readByte1(empty_rxbuf,r_cnt);
				//	spi_write_read_clean();
					r_total -=r_cnt;
				//	bl_printf("r_total6 = %d,r_cnt = %d\r\n",r_total,r_cnt);
				}

			}

			//spi_write_read_clean();
			max_len -= r_cnt;
			if(get_5mstime_cnt() > 2000)
			{
				status = FLASH_OP_T_OUT;
				goto ret;
			}
		}


ret:

		spi_tx_end();

		spi_write_read_clean();


		return status;
}



uint8_t spi_write_read(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset)
{
		uint32_t max_len;
		uint8_t status;

		uint8_t empty_txbuf[SPI_FIFO_LEN] = {0};

		uint8_t empty_rxbuf[SPI_FIFO_LEN] = {0};

		memset(empty_txbuf,0,SPI_FIFO_LEN);

		max_len = (w_size > (r_size + r_offset)) ? w_size : (r_size + r_offset);

		status = STATUS_OK;
		spi_tx_start();

		while(max_len > SPI_FIFO_LEN)
		{
			if(w_size > SPI_FIFO_LEN)
			{
				status = spi_writeByte1(wbuf,SPI_FIFO_LEN);
				wbuf +=SPI_FIFO_LEN ;
				w_size -= SPI_FIFO_LEN;
			}else if(w_size > 0)
			{
				status = spi_writeByte1(wbuf,w_size);
				status = spi_writeByte1(empty_txbuf,SPI_FIFO_LEN - w_size);
				wbuf +=w_size ;
				w_size-=w_size;
			}else
			{
				status = spi_writeByte1(empty_txbuf,SPI_FIFO_LEN);
			}


			if(r_offset > SPI_FIFO_LEN)
			{
				status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN);
				r_offset-= SPI_FIFO_LEN;
			}else if (r_offset > 0)
			{
				status = spi_readByte1(empty_rxbuf,r_offset);
				status = spi_readByte1(rbuf,SPI_FIFO_LEN - r_offset);
				rbuf +=  (SPI_FIFO_LEN - r_offset);
				r_size -=  (SPI_FIFO_LEN - r_offset);
				r_offset = 0;

			}
			else if(r_size > SPI_FIFO_LEN)
			{
				status = spi_readByte1(rbuf,SPI_FIFO_LEN);
				rbuf+= SPI_FIFO_LEN;
				r_size -= SPI_FIFO_LEN;
			}
			else if(r_size > 0)
			{
					status = spi_readByte1(rbuf,r_size);
					status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN - r_size);
					rbuf+= r_size;
					r_size  = 0;
			}else
			{

			//	status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN);
			//	spi_write_read_clean();
			}

			max_len -= SPI_FIFO_LEN;

			if(status != STATUS_OK)
			{
				goto ret;
			}
		}

		if(max_len > 0)
		{
			if(w_size > 0)
			{
				memcpy(empty_txbuf,wbuf,w_size);
				status = spi_writeByte1(empty_txbuf,max_len);


				status = spi_readByte1(empty_rxbuf,max_len);
				if(r_size > 0)
				{
					memcpy(rbuf,&(empty_rxbuf[r_offset]),r_size);
				}

			}else
			{
				status = spi_writeByte1(empty_txbuf,max_len);

				status = spi_readByte1(empty_rxbuf,max_len);

				if(r_size > 0)
				{
					memcpy(rbuf,&(empty_rxbuf[r_offset]),r_size);
				}


			}


			if(status != STATUS_OK)
			{
				goto ret;
			}
		}

ret:
		spi_tx_end();


		spi_write_read_clean();


		return status;
}


uint8_t spi_write_read_no_cs(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset)
{
		uint32_t max_len;
		uint8_t status;

		uint8_t empty_txbuf[SPI_FIFO_LEN] = {0};

		uint8_t empty_rxbuf[SPI_FIFO_LEN] = {0};

		memset(empty_txbuf,0,SPI_FIFO_LEN);

		max_len = (w_size > (r_size + r_offset)) ? w_size : (r_size + r_offset);

		status = STATUS_OK;
	//	spi_tx_start();

		while(max_len > SPI_FIFO_LEN)
		{
			if(w_size > SPI_FIFO_LEN)
			{
				status = spi_writeByte1(wbuf,SPI_FIFO_LEN);
				wbuf +=SPI_FIFO_LEN ;
				w_size -= SPI_FIFO_LEN;
			}else if(w_size > 0)
			{
				status = spi_writeByte1(wbuf,w_size);
				status = spi_writeByte1(empty_txbuf,SPI_FIFO_LEN - w_size);
				wbuf +=w_size ;
				w_size-=w_size;
			}else
			{
				status = spi_writeByte1(empty_txbuf,SPI_FIFO_LEN);
			}


			if(r_offset > SPI_FIFO_LEN)
			{
				status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN);
				r_offset-= SPI_FIFO_LEN;
			}else if (r_offset > 0)
			{
				status = spi_readByte1(empty_rxbuf,r_offset);
				status = spi_readByte1(rbuf,SPI_FIFO_LEN - r_offset);
				rbuf +=  (SPI_FIFO_LEN - r_offset);
				r_size -=  (SPI_FIFO_LEN - r_offset);
				r_offset = 0;

			}
			else if(r_size > SPI_FIFO_LEN)
			{
				status = spi_readByte1(rbuf,SPI_FIFO_LEN);
				rbuf+= SPI_FIFO_LEN;
				r_size -= SPI_FIFO_LEN;
			}
			else if(r_size > 0)
			{
					status = spi_readByte1(rbuf,r_size);
					status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN - r_size);
					rbuf+= r_size;
					r_size  = 0;
			}else
			{

			//	status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN);
			//	spi_write_read_clean();
			}

			max_len -= SPI_FIFO_LEN;

			if(status != STATUS_OK)
			{
				goto ret;
			}
		}

		if(max_len > 0)
		{
			if(w_size > 0)
			{
				memcpy(empty_txbuf,wbuf,w_size);
				status = spi_writeByte1(empty_txbuf,max_len);


				status = spi_readByte1(empty_rxbuf,max_len);
				if(r_size > 0)
				{
					memcpy(rbuf,&(empty_rxbuf[r_offset]),r_size);
				}

			}else
			{
				status = spi_writeByte1(empty_txbuf,max_len);

				status = spi_readByte1(empty_rxbuf,max_len);

				if(r_size > 0)
				{
					memcpy(rbuf,&(empty_rxbuf[r_offset]),r_size);
				}


			}

			if(status != STATUS_OK)
			{
				goto ret;
			}

		}

	//	spi_tx_end();
ret:

		spi_write_read_clean();


		return status;
}


uint8_t spi_write_read_4K_W(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset)
{
		uint32_t max_len;
		uint8_t status;

		uint8_t empty_txbuf[SPI_FIFO_LEN] = {0};

		uint8_t empty_rxbuf[SPI_FIFO_LEN] = {0};

		memset(empty_txbuf,0,SPI_FIFO_LEN);

		max_len = (w_size > (r_size + r_offset)) ? w_size : (r_size + r_offset);

		status = STATUS_OK;
		spi_tx_start();

		while(max_len > SPI_FIFO_LEN)
		{
			if(w_size > SPI_FIFO_LEN)
			{
				status = spi_writeByte1(wbuf,SPI_FIFO_LEN);
				wbuf +=SPI_FIFO_LEN ;
				w_size -= SPI_FIFO_LEN;
			}else if(w_size > 0)
			{
				status = spi_writeByte1(wbuf,w_size);
				status = spi_writeByte1(empty_txbuf,SPI_FIFO_LEN - w_size);
				wbuf +=w_size ;
				w_size-=w_size;
			}else
			{
				status = spi_writeByte1(empty_txbuf,SPI_FIFO_LEN);
			}


			if(r_offset > SPI_FIFO_LEN)
			{
				status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN);
				r_offset-= SPI_FIFO_LEN;
			}else if (r_offset > 0)
			{
				status = spi_readByte1(empty_rxbuf,r_offset);
				status = spi_readByte1(rbuf,SPI_FIFO_LEN - r_offset);
				rbuf +=  (SPI_FIFO_LEN - r_offset);
				r_size -=  (SPI_FIFO_LEN - r_offset);
				r_offset = 0;

			}
			else if(r_size > SPI_FIFO_LEN)
			{
				status = spi_readByte1(rbuf,SPI_FIFO_LEN);
				rbuf+= SPI_FIFO_LEN;
				r_size -= SPI_FIFO_LEN;
			}
			else if(r_size > 0)
			{
					status = spi_readByte1(rbuf,r_size);
					status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN - r_size);
					rbuf+= r_size;
					r_size  = 0;
			}else
			{

			//	status = spi_readByte1(empty_rxbuf,SPI_FIFO_LEN);
			//	spi_write_read_clean();
			}

			max_len -= SPI_FIFO_LEN;
			if(status != STATUS_OK)
			{
				goto ret;
			}

		}

		if(max_len > 0)
		{
			if(w_size > 0)
			{
				memcpy(empty_txbuf,wbuf,w_size);
				status = spi_writeByte1(empty_txbuf,max_len);


				status = spi_readByte1(empty_rxbuf,max_len);
				if(r_size > 0)
				{
					memcpy(rbuf,&(empty_rxbuf[r_offset]),r_size);
				}

			}else
			{
				status = spi_writeByte1(empty_txbuf,max_len);

				status = spi_readByte1(empty_rxbuf,max_len);

				if(r_size > 0)
				{
					memcpy(rbuf,&(empty_rxbuf[r_offset]),r_size);
				}


			}


			if(status != STATUS_OK)
			{
				goto ret;
			}

		}
ret:
		spi_tx_end();


		spi_write_read_clean();


		return status;
}

uint8_t spi_is_busy(void)
{

     return 0 ;

}



