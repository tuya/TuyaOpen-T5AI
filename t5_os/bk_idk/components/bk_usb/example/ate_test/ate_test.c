#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <driver/int_types.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "sys_driver.h"
#include "sys_types.h"
#include "sys_rtos.h"
#include "ate_test.h"

#define USB_BASE_ADDR           SOC_USB_REG_BASE
#define REG_USB_USR_708                (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x708)))
#define REG_USB_USR_70C                (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x70C)))
#define REG_USB_USR_710                (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x710)))

#define NANENG_REG_USB_PHY_00                 (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x400)))
#define NANENG_REG_USB_PHY_01                 (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x404)))
#define NANENG_REG_USB_PHY_02                 (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x408)))
#define NANENG_REG_USB_PHY_0D                 (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x434)))

#define NANENG_REG_USB_PHY_0F                 (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x43C)))
#define NANENG_REG_USB_PHY_1C                 (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x470)))

#define SYS_REG_CLOCK_0C                      (*((volatile unsigned long *)   (0x44010000 + 0xc * 4)))
#define SYS_REG_DPDN_CAP_43                   (*((volatile unsigned long *)   (0x44010000 + 0x43 * 4)))

void bk_set_ate_self_test(void)
{
	/* Set SELF_TEST high and RESET high */
	REG_USB_USR_710 |= (0x1<<15);
	//REG_USB_USR_710 |= (0x1<<14);
	REG_USB_USR_710 |= (0x1<<16);
	REG_USB_USR_710 |= (0x1<<17);
	REG_USB_USR_710 |= (0x1<<18);
	REG_USB_USR_710 |= (0x1<<19);
	REG_USB_USR_710 &=~(0x1<<20);
	REG_USB_USR_710 |= (0x1<<21);
	REG_USB_USR_710 |= (0x0<< 0);
	REG_USB_USR_710 |= (0x1<< 5);
	REG_USB_USR_710 |= (0x1<< 6);
	REG_USB_USR_710 |= (0x1<< 9);
	REG_USB_USR_710 |= (0x1<<10);
	REG_USB_USR_710 |= (0x1<< 7);
	REG_USB_USR_710 |= (0x1<< 1);
	REG_USB_USR_708  =	0x1;
}

bk_err_t bk_usb_ate_bist_test(uint32_t state)
{
	uint32_t reg = 0;
	extern void bk_delay(INT32 num);
	if(state == 1) {
	gpio_dev_unprotect_unmap(12);
	gpio_dev_unprotect_unmap(13);
	gpio_dev_unprotect_map(12, GPIO_DEV_USB0_DP);
	gpio_dev_unprotect_map(13, GPIO_DEV_USB0_DN);

	/* Make sure of properly connection for all power and ground pins */
	reg = *((volatile unsigned long *) (0x44010000+0x10*4));
	reg &= ~(1<<5);
	*((volatile unsigned long *) (0x44010000+0x10*4)) = reg;//0.9v
	
	bk_delay(1000);
	*((volatile unsigned long *) (0x44010000+0x4d*4)) |= (1<<31);//buckpa enable
	bk_delay(1000);
	*((volatile unsigned long *) (0x44010000+0x4a*4)) |= (1<<12);//1.8v
	bk_delay(1000);
	*((volatile unsigned long *) (0x44010000+0x4a*4)) |= (1<<13);//3.3v
	bk_delay(1000);

	/* Input REFCLK into IP with frequency of 12MHz set REFCLK_MODE to high*/
	//sys_drv_usb_clock_ctrl(true, NULL);
	SYS_REG_CLOCK_0C |= (0x1<<17);
	bk_set_ate_self_test();

	/* Set RESET low to start the self-test function */
	NANENG_REG_USB_PHY_00 = 0x08;	 
	NANENG_REG_USB_PHY_01 = 0x02;
	REG_USB_USR_710 |= (0x1<< 8);
	/* After ~1ms Test-BIST will be high */
	uint32_t waitready_count = 0;
	while(1){
		reg = REG_USB_USR_70C;
		if(reg & 0x100){
			USB_ATE_LOGI("SelfTest Fin!\r\n");
			USB_ATE_LOGI("test end!\r\n");
			break;
		} else {
			if(waitready_count > 20) return BK_FAIL;
			USB_ATE_LOGI("70c_reg:0x%x\r\n", reg);
			waitready_count++;
			bk_delay(10000);
		}
	}
	NANENG_REG_USB_PHY_00 &= ~0x08;	   
	NANENG_REG_USB_PHY_01 &= ~0x02;
	REG_USB_USR_710 &= ~(0x1<< 8);
	}else {

		reg = *((volatile unsigned long *) (0x44010000+0x4a*4));
		reg &= (1<<13);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) = reg;//3.3v
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x4a*4));
		reg &= ~(1<<12);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) = reg;//1.8v
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x4d*4));
		reg &= ~(1<<31);
		*((volatile unsigned long *) (0x44010000+0x4d*4)) = reg;//buckpa enable
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x10*4));
		reg &= ~(0<<5);
		*((volatile unsigned long *) (0x44010000+0x10*4)) = reg;//0.9v

	}

	return BK_OK;
}

bk_err_t bk_usb_ate_voh_vol_test(uint32_t state)
{
	uint32_t reg = 0;
	extern void bk_delay(INT32 num);

	if(state == 1) {
		gpio_dev_unprotect_unmap(12);
		gpio_dev_unprotect_unmap(13);
		gpio_dev_unprotect_map(12, GPIO_DEV_USB0_DP);
		gpio_dev_unprotect_map(13, GPIO_DEV_USB0_DN);
		SYS_REG_DPDN_CAP_43 |= 0xFF;
		//sys_drv_usb_clock_ctrl(true, NULL);
		SYS_REG_CLOCK_0C |= (0x1<<17);

		/* Power up VCCA33 VCCA18 VDDA */
		reg = *((volatile unsigned long *) (0x44010000+0x10*4));
		reg &= ~(1<<5);
		*((volatile unsigned long *) (0x44010000+0x10*4)) = reg;//0.9v
		
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4d*4)) |= (1<<31);//buckpa enable
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) |= (1<<12);//1.8v
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) |= (1<<13);//3.3v
		bk_delay(1000);
		bk_set_ate_self_test();
		//Configure the register pin "cfg_test_mode" to high, enable the CP test
		//NANENG_REG_USB_PHY_0D |= (0x1 << 3);
	} else if(state == 0){
		NANENG_REG_USB_PHY_00 &= ~(0x1 << 3);
		reg = *((volatile unsigned long *) (0x44010000+0x4a*4));
		reg &= (1<<13);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) = reg;//3.3v
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x4a*4));
		reg &= ~(1<<12);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) = reg;//1.8v
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x4d*4));
		reg &= ~(1<<31);
		*((volatile unsigned long *) (0x44010000+0x4d*4)) = reg;//buckpa enable
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x10*4));
		reg &= ~(0<<5);
		*((volatile unsigned long *) (0x44010000+0x10*4)) = reg;//0.9v
	}else if(state == 2) {
		//Configure the register pin "cfg_hs_test_sel[1:0]" to 01, start Voh test
		NANENG_REG_USB_PHY_0D = (0x98);

		//Test the voltage value of "DP" for Voh_dp and test the voltage value of DM for Vol_dm
	} else if(state == 3) {
		//Configure the register pin "cfg_hs_test_sel[1:0]" to 10, start Vol test
		NANENG_REG_USB_PHY_0D = (0xA8);

		//Test the voltage of "DP" for Vol_dp and test the voltage value of DM for Voh_dm
	}
	return BK_OK;

}

bk_err_t bk_usb_ate_rterm_test(uint32_t state)
{
	uint32_t reg = 0;
	extern void bk_delay(INT32 num);

	if(state == 1) {
		gpio_dev_unprotect_unmap(12);
		gpio_dev_unprotect_unmap(13);
		gpio_dev_unprotect_map(12, GPIO_DEV_USB0_DP);
		gpio_dev_unprotect_map(13, GPIO_DEV_USB0_DN);
		SYS_REG_DPDN_CAP_43 |= 0xFF;
		//sys_drv_usb_clock_ctrl(true, NULL);
		SYS_REG_CLOCK_0C |= (0x1<<17);
		/* Power up VCCA33 VCCA18 VDDA */
		reg = *((volatile unsigned long *) (0x44010000+0x10*4));
		reg &= ~(1<<5);
		*((volatile unsigned long *) (0x44010000+0x10*4)) = reg;//0.9v
		
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4d*4)) |= (1<<31);//buckpa enable
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) |= (1<<12);//1.8v
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) |= (1<<13);//3.3v
		bk_delay(1000);
		bk_set_ate_self_test();
		//Configure the register pin "cfg_test_mode" to high, enable the CP test
		//NANENG_REG_USB_PHY_0D |= (0x1 << 3);
	} else if(state == 0){
		NANENG_REG_USB_PHY_0D &= ~(0x1 << 3);
		reg = *((volatile unsigned long *) (0x44010000+0x4a*4));
		reg &= (1<<13);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) = reg;//3.3v
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x4a*4));
		reg &= ~(1<<12);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) = reg;//1.8v
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x4d*4));
		reg &= ~(1<<31);
		*((volatile unsigned long *) (0x44010000+0x4d*4)) = reg;//buckpa enable
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x10*4));
		reg &= ~(0<<5);
		*((volatile unsigned long *) (0x44010000+0x10*4)) = reg;//0.9v
	}else if(state == 2) {
		//Configure the register pin "cfg_hs_test_sel[1:0]" to 00, start Rterm
		NANENG_REG_USB_PHY_0D = 0x88;
	}

	//Connect a voltage source Vtest(400mV) to DP/DM

	//Test the output current Itest of the voltage source

	//Calculate the Rterm, Rterm is Vtest/Itest
	return BK_OK;

}

bk_err_t bk_usb_ate_rx_dc_input_test(uint32_t state)
{
	uint32_t reg = 0;
	extern void bk_delay(INT32 num);

	if(state == 1) {
		gpio_dev_unprotect_unmap(12);
		gpio_dev_unprotect_unmap(13);
		gpio_dev_unprotect_map(12, GPIO_DEV_USB0_DP);
		gpio_dev_unprotect_map(13, GPIO_DEV_USB0_DN);
		/* Input REFCLK into IP with frequency of 12MHz set REFCLK_MODE/PLL to high*/
		//sys_drv_usb_clock_ctrl(true, NULL);
		SYS_REG_CLOCK_0C |= (0x1<<17);
		SYS_REG_DPDN_CAP_43 |= 0xFF;
		/* Power up VCCA33 VCCA18 VDDA */
		reg = *((volatile unsigned long *) (0x44010000+0x10*4));
		reg &= ~(1<<5);
		*((volatile unsigned long *) (0x44010000+0x10*4)) = reg;//0.9v
		
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4d*4)) |= (1<<31);//buckpa enable
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) |= (1<<12);//1.8v
		bk_delay(1000);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) |= (1<<13);//3.3v
		bk_delay(1000);
		bk_set_ate_self_test();
		//Configure the register pin "cfg_test_mode" to high, enable the RX DC test
		//NANENG_REG_USB_PHY_0D |= (0x1 << 3);
	} else if(state == 0){
		NANENG_REG_USB_PHY_00 &= ~(0x1 << 3);
		reg = *((volatile unsigned long *) (0x44010000+0x4a*4));
		reg &= (1<<13);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) = reg;//3.3v
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x4a*4));
		reg &= ~(1<<12);
		*((volatile unsigned long *) (0x44010000+0x4a*4)) = reg;//1.8v
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x4d*4));
		reg &= ~(1<<31);
		*((volatile unsigned long *) (0x44010000+0x4d*4)) = reg;//buckpa enable
		bk_delay(10000);

		reg = *((volatile unsigned long *) (0x44010000+0x10*4));
		reg &= ~(0<<5);
		*((volatile unsigned long *) (0x44010000+0x10*4)) = reg;//0.9v
	}else if(state == 2) {
		//Configure the register pin "cfg_hsrx_test" to high, enable RX blocks, wait about 200us for RX blocks finish the set-up
		NANENG_REG_USB_PHY_0F |= (0x1 << 4);
		bk_delay(250);
	}else if(state == 3) {
		//Input a voltage source Vinrx between DP and DM, the initial voltage of Vinrx is 0V
		//Increase or decrease the voltage of Vinrx at 5mV/us speed until "hsrx_test_o" is stable to 0/1
		uint32_t waitready_count = 0;
		while(1){
			reg = NANENG_REG_USB_PHY_1C;
			if(reg & 0x4){
				USB_ATE_LOGI("SelfTest Fin!\r\n");
				USB_ATE_LOGI("test end!\r\n");
				break;
			} else {
				if(waitready_count > 20) return BK_FAIL;
				USB_ATE_LOGI("70c_reg:0x%x\r\n", reg);
				waitready_count++;
				bk_delay(10000);
			}
		}
	}
	//Test the Vinrx at the point of "hsrx_test_o" changing from un-stable to stable, the value of Vinrx is the RX minimum input voltage
	return BK_OK;

}


