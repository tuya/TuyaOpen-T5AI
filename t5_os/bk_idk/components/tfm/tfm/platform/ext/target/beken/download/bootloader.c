#include "type.h"
#include "bl_config.h"
#include "system.h"
#include "bootloader.h"
#include "bl_extflash.h"
#include "bl_spi.h"
#include "bl_bk_hci_protocol.h"
#include "bl_uart.h"
#include "pal_log.h"
#include "hal_platform.h"
#include "cmsis.h"

#if (CPU_RISCV)
#include "int.h"
#endif

#if (CPU_ARM)
#include "intc.h"
#endif

#if (CPU_BA22)
#include "exception.h"
#include "spr_defs.h"
#endif

/***************************************************CHANGE ***************************/


//#define cpu1_vector    (*((volatile unsigned long *)0x44010014))


void BK3000_ICU_Initial(void)
{
#if (CHIP_BK3281)
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_UART_PWD;
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_SPI_PWD;

    BK3000_PMU_PERINT_ENABLE = 0x2;
#endif
	
#if (CHIP_BK3266)
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_UART_PWD;
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_SPI_PWD;
//	BK3000_PMU_PERI_PWDS &= ~bit_PMU_CEVA_PWD; // enable CEVA baseband clock
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_LPO_CLK_STEAL_ENABLE; // Clock Stealing Enable to get 32k clock
	BK3000_PMU_PERI_PWDS |= bit_PMU_LPO_CLK_SEL_XTAL; // 1:XTAL?? 0:ROSC?? 32.768
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_LPO_CLK_DISABLE; // enable LPO clock using for 32K clock
	BK3000_PMU_PERI_PWDS |= bit_PMU_CLK_FREQ_SEL;

#endif
	
#if (CHIP_BK3288)

	SET_PERIxCLK(UART0CLK_PWD)         ;
	SET_PERIxCLK(SPI0CLK_PWD)          ;
	addSYSTEM_Reg0x9 |=  INT_ID_UART0  ; 
	set_spi_rw_flash                   ;

	clr_ram_sd(uart0_ram_sd);
	clr_ram_sd(spi0_ram_sd);
#endif

#if (CHIP_BK3633)

UART0_ENABLE      ;
SPI0_ENABLE       ;
UART0_INT_ENABLE  ;
SET_SPI_RW_FLASH  ;

#endif


#if (CHIP_BK7256)

UART0_ENABLE      ;
SPI0_ENABLE       ;
UART0_INT_ENABLE  ;
SET_SPI_RW_FLASH  ;

#endif

#if (CHIP_BK7236)
	UART0_RESET;
	UART0_ENABLE;
	SPI0_ENABLE;
	UART0_INT_ENABLE;
	SET_SPI_RW_FLASH;
#endif
}


void control_set_to_flash_not_enable_jtag()
{
#if (CHIP_BK3281)
	 BK3000_PMU_ROM_FLASH_SECT = 0;
#endif

#if (CHIP_BK3266)
	 BK3000_PMU_ROM_FLASH_SECT |= ( 0X01 << 0);
#endif

#if (CHIP_BK3288)
	 BK3000_PMU_ROM_FLASH_SECT &= ~(0x01<<0) ;
#endif
	
#if (CHIP_BK3633)	
	SET_FLASHCTRL_RW_FLASH ;
	REBOOT ;
#endif	

#if (CHIP_BK7256)
	SET_FLASHCTRL_RW_FLASH ;
	REBOOT ;
#endif

#if (CHIP_BK7236)
	//SET_FLASHCTRL_RW_FLASH ;
	//REBOOT;
	*(volatile uint32_t *)0x44000600 = 0x5A000A;
	*(volatile uint32_t *)0x44000600 = 0xA5000A;
	while(1);
#endif
}

void control_set_to_flash()
{
	//hal_jtag_enable();
	control_set_to_flash_not_enable_jtag();
}

extern u16 COUNT;
void uart_enable(uint8_t flag,uint8_t rx_thr) {

	if (flag == 0)
		boot_uart_init(UART_BAUDRATE_DEFAULE,rx_thr);
	else
		boot_uart_init(UART_BAUDRATE_DEFAULE,rx_thr);//UART_BAUDRATE_2000000

#if (CHIP_BK3281)
	BK3000_GPIO_11_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 3 << 3; // only cfg Rx
#endif
	
#if (CHIP_BK3266)
		BK3000_GPIO_1_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3; // only cfg Rx
#endif

#if (CHIP_BK3288)
		BK3000_GPIO_1_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
#endif
	
#if (CHIP_BK3633)
		BK3000_GPIO_1_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
#endif	

#if (CHIP_BK7256)
		//BK3000_GPIO_11_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
		BK3000_GPIO_10_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
#endif

#if (CHIP_BK7236)
	BK3000_GPIO_10_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
#endif
}


void uart_tx_pin_cfg(void)
{
#if (CHIP_BK3281)
	BK3000_GPIO_10_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE ;
#endif
	
#if (CHIP_BK3266)
	BK3000_GPIO_0_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif
	
#if (CHIP_BK3288)
	BK3000_GPIO_0_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif
	
#if (CHIP_BK3633)
	BK3000_GPIO_0_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif	

#if (CHIP_BK7256)
	//BK3000_GPIO_10_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
	BK3000_GPIO_11_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif
#if (CHIP_BK7236)
	BK3000_GPIO_11_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif

}



void PMU_uninit( void )
{
#if (CHIP_BK3266)
	BK3000_PMU_PERI_PWDS |= bit_PMU_UART_PWD;
	BK3000_PMU_PERI_PWDS |= bit_PMU_SPI_PWD;
#endif

#if (CHIP_BK3288)
	CLR_PERIxCLK(UART0CLK_PWD)           ;
	CLR_PERIxCLK(SPI0CLK_PWD)            ;
	addSYSTEM_Reg0x9 &=  ~ INT_ID_UART0  ;
	set_flashctrl_rw_flash               ;
	set_ram_sd(uart0_ram_sd)             ;
	set_ram_sd(spi0_ram_sd)              ;
#endif
	
#if (CHIP_BK3633)	
	 UART0_DISABLE      ;
	 SPI0_DISABLE       ;
	 UART0_INT_DISABLE  ;
//	 IRQ_DISABLE        ;
#else
	 UART0_DISABLE      ;
	 SPI0_DISABLE       ;
	 UART0_INT_DISABLE  ;
	SET_FLASHCTRL_RW_FLASH;
#endif		
	
}


void BK3000_start_wdt(uint32_t val) {
	
#if (CHIP_BK3633)	
	AON_WDT_PERIOD  = val  ;
	AON_WDT_FEED    = 0x5A ;
	AON_WDT_FEED    = 0xA5 ;	
#elif (CHIP_BK7256)
	BK3000_WDT_CONFIG = 0x5A0000 | val;
	BK3000_WDT_CONFIG = 0xA50000 | val;
	AON_WDT_CONFIG = 0x5A0000 | val;
	AON_WDT_CONFIG = 0xA50000 | val;
 #elif (CHIP_BK7236)
	/* On FPGA, the WDT clock is 25Mhz, it will trigger watchdog soon,
	 * since FPGA doesn't support reset, it triggers NMI instead of
	 * WDT reset.
	 * */
#if CONFIG_WDT
	BK3000_WDT_CONTROL = 0x3;
	BK3000_WDT_CONFIG = 0x5A0000 | val;
	BK3000_WDT_CONFIG = 0xA50000 | val;

	AON_WDT_CONFIG = 0x5A0000 | val;
	AON_WDT_CONFIG = 0xA50000 | val;
#endif
#endif	
}









/************************************CHANGE ***************************************************/
/***************************************************************************************/
void uart_disable(void) {
	REG_APB3_UART_CFG = 0;
}

void arch_enable_uart_int(void)
{
        NVIC_EnableIRQ(4);
}

void SYSirq_Initialise(void) {
	arch_enable_uart_int();
}

void bl_init(void) {
	bsp_initial();
	SYSirq_Initialise();
}





#ifdef FLASH_DEBUG
void set_flash_clk(unsigned char clk_conf) {
    unsigned int temp0;
    temp0 = reg_FLASH_CONF;
    reg_FLASH_CONF = (  (clk_conf << BIT_FLASH_CLK_CONF)
                      | (temp0    &  SET_MODE_SEL)
                      | (temp0    &  SET_FWREN_FLASH_CPU)
                      | (temp0    &  SET_WRSR_DATA)
                      | (temp0    &  SET_CRC_EN));
}
#endif

void watchdog_stop(void) {
	BK3000_WDT_CONFIG = 0x5A0000;
	BK3000_WDT_CONFIG = 0xA50000;
	AON_WDT_CONFIG = 0x5A0000;
	AON_WDT_CONFIG = 0xA50000;
	return;
}



void BK3000_ICU_Initial(void);
void bsp_initial(void)
{

	BK3000_ICU_Initial();

}
extern u8 uart_link_check_flag ;

#define addEFUSE_Reg0x2                                         *((volatile unsigned long *) (0x44880000+0x2*4))
#define addEFUSE_Reg0x4                                         *((volatile unsigned long *) (0x44880000+0x4*4))
#define addEFUSE_Reg0x5                                         *((volatile unsigned long *) (0x44880000+0x5*4))
#define get_EFUSE_Reg0x1_efuse_read_data_valid                  ((addEFUSE_Reg0x5 & 0x100) >> 8)

void deep_sleep_to_flash()
{
	PAL_LOG_DEBUG("deep sleep rst, pmu reg0=%x\n", BK3000_PMU_RESET_REASON_GET);
	if( DEEP_SLEEP_RESTART )//restart from deepsleep ,so go to flash
	{
#if(CHIP_BK7256)
    		addEFUSE_Reg0x0 = 0x01 | (0x1e << 8);
    		while(get_EFUSE_Reg0x1_efuse_read_data_valid == 0);

		if( (addEFUSE_Reg0x1 & 0x4) == 0 ){    //efuse 0x1e[2] 0: deepsleep fast boot 1: deepsleep sb boot
    			control_set_to_flash();
    		}
#elif(CHIP_BK7236)
		addEFUSE_Reg0x2 = 0x03;
		addEFUSE_Reg0x4 = 0x01 | (0x0 << 8);
		while(get_EFUSE_Reg0x1_efuse_read_data_valid == 0);

		if( (addEFUSE_Reg0x5 & 0x4) == 0 ){    //efuse 0x0[2] 0: deepsleep fast boot 1: deepsleep sb boot
			control_set_to_flash();
		}
#else
		control_set_to_flash() ;
#endif
	}
}


extern u_int8  Longtrx_pdu_buf[4200];
extern volatile u_int8* tra_hcit_rx_pdu_buf;

extern void dbg_printf_init(void);
extern void sb_boot(void);
u8 *g_data_buf4k;

void legacy_boot_main(void) {

	int restart_t;
	unsigned int delay_time;

/*		 // cold restart */

	{
		delay_time = 0;
		BK3000_start_wdt(0xA000);
	//	watchdog_stop();
		//1. check flash is empty ?

		//sb_boot();

		bl_init();

		uart_enable(1,12);
#if GPIO_DEBUG
		gpio_debug_init(5);
#endif
		spi_initial(SPI_MASTER,SPI_DEFAULT_BAUD);
		//restart_t = ext_flash_is_empty();
		restart_t = 0x5;
		g_data_buf4k = (u8 *)pal_malloc(0x1100 * sizeof(u8));
		if (g_data_buf4k == NULL) {
			return;
		}
		if(restart_t) // !0: flash is not empty 0: empty ;
		{
		//	u8 c = 0xaa;
			if(restart_t == 0xe1)
				  restart_t = 0 ;
			u32 i = DELAY_1MS_CNT * restart_t;//restart_t;//restart_t; // one 2us ;500 1ms

			while(i--)//wait 5ms
			{
				TRAhcit_UART_Rx();
				BK3000_start_wdt(0xA000);

				if(uart_link_check_flag == 1)
				{
				//	bl_printf("uart_link_check OK!!!\r\n");
				//	uart_link_check_flag = 0;

					boot_uart_init(UART_BAUDRATE_DEFAULE,RX_FIFO_THR_COUNT);
					tra_hcit_rx_pdu_buf = (volatile u_int8*)Longtrx_pdu_buf;
					uart_static_read85();
					while(1)
					{

						TRAhcit_UART_Rx();
						BK3000_start_wdt(0xA000);
					}
				}

				//bk_delay(10);
			}
		//	c = 0xbb;
			//debug2_uart_send_poll(&c,1);


#if 1
			PMU_uninit();
			BK3000_start_wdt(0xA000);
			goto LEGACY_BOOT_FINISH;

#else
			//2.go to flash
			PMU_uninit();
			BK3000_start_wdt(0xA000);
			control_set_to_flash();

			while(1);
#endif

		}else
		{
			boot_uart_init(UART_BAUDRATE_DEFAULE,RX_FIFO_THR_COUNT);
			uart_tx_pin_cfg();
			tra_hcit_rx_pdu_buf = (volatile u_int8*)Longtrx_pdu_buf;
			uart_static_read85();
			while(1)
			{
				TRAhcit_UART_Rx();
//				BK3000_start_wdt(0xA000);
				if((uart_link_check_flag == 0) && (delay_time == 0 )  ){
					restart_t = ext_flash_is_empty();
					if(restart_t > 0 )   delay_time =  restart_t * DELAY_1MS_CNT ;//restart_t;//restart_t; // one 2us ;500 1ms
				}
				if(delay_time > 0)  delay_time--;
			    if((delay_time == 1) && (uart_link_check_flag == 0)) {      //once delay_time >0 and no_uart link,  after delay_time enter flash mode
						PMU_uninit();
						BK3000_start_wdt(0xA008);
						goto LEGACY_BOOT_FINISH;
						//control_set_to_flash();
						//while(1);
				}
			    while(uart_link_check_flag == 1)  {
			    	BK3000_start_wdt(0xA006);
			        TRAhcit_UART_Rx();
			    }
			}
		}

	}
LEGACY_BOOT_FINISH:
	free(g_data_buf4k);
	return;
}



// EOF
