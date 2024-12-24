#include "bkreg.h"
#include "drv_system.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "bkstdio.h"

int bk_printf(const char *fmt, ...);

#if     defined(__BA2__)
#define DIV     6
#define NOP()   __asm__("b.nop 5;")
#elif   defined(CEVAX2)
#define DIV     4
#define NOP()   __asm__("PCU.nop;")
#elif   defined(TEAKLITE4)
#define DIV     6
#define NOP()   __asm__("nop;")
#else
#define DIV     6
#define NOP()   __asm__("nop")
#endif

void sys_delay_cycle(uint32_t n)
{
    while(n--) NOP();
}

void sys_delay_us(uint32_t us)
{
    sys_delay_cycle(50 * us);
}

void sys_delay_ms(uint32_t ms)
{
    sys_delay_cycle(50 * 1000 * ms);
}

void system_reg_write(uint32_t index, uint32_t value)
{
    (*(volatile unsigned long*)(MDU_SYSTEM_BASE_ADDR + index * 4)) = value;

    if(index >= 0x40 && index <= 0x55)
    {
        while((*(volatile unsigned long*)(MDU_SYSTEM_BASE_ADDR + 0x0B * 4)) >> ((index - 0x40) + 8));
    }
}

void system_init(void)
{
    uint32_t reg = REG_SYSTEM_0x45;
	reg |=  (0x1 << 14) | (0x1 << 5) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1);
	system_reg_write(0x45, reg);

	reg = REG_SYSTEM_0x40;
	reg &= ~(0x1F << 20);
    reg |=  (0x13 << 20);
	system_reg_write(0x40, reg);

	system_reg_write(0x40, 0xF1305B57);
	system_reg_write(0x40, 0xF5305B57);
	system_reg_write(0x40, 0xF1305B57);
	system_reg_write(0x42, 0x06005450);
	system_reg_write(0x43, 0xC5F00B88);
	system_reg_write(0x48, 0x57E62726);
	system_reg_write(0x49, 0x787BC6A4);
	system_reg_write(0x4A, 0xB215C3A7);
	system_reg_write(0x4B, 0x9FEF31FF);
	system_reg_write(0x4C, 0x9F03EF6F);
	system_reg_write(0x4D, 0x1F6FB3FE);

    reg  = REG_SYSTEM_0x08;
    reg &= ~(MSK_SYSTEM_0x08_CLKDIV_CORE | MSK_SYSTEM_0x08_CKSEL_CORE);
    reg |= (3 << SFT_SYSTEM_0x08_CLKDIV_CORE) | (3 << SFT_SYSTEM_0x08_CKSEL_CORE);
    REG_SYSTEM_0x08 = reg;

    reg  = REG_SYSTEM_0x09;
    reg &= ~(MSK_SYSTEM_0x09_CKSEL_FLASH | MSK_SYSTEM_0x09_CKDIV_FLASH);
    reg |= (1 << SFT_SYSTEM_0x09_CKSEL_FLASH) | (0 << SFT_SYSTEM_0x09_CKDIV_FLASH);
    REG_SYSTEM_0x09 = reg;
}

void system_peri_clk_enable(SYS_PERI_CLK peries)
{
    REG_SYSTEM_0x0D |= (uint32_t)(peries >> 32);
    REG_SYSTEM_0x0C |= (uint32_t)(peries & 0xFFFFFFFF);
}

void system_peri_clk_disable(SYS_PERI_CLK peries)
{
    REG_SYSTEM_0x0D &= ~(uint32_t)(peries >> 32);
    REG_SYSTEM_0x0C &= ~(uint32_t)(peries & 0xFFFFFFFF);
}

void system_peri_cpu0_irq_enable(SYS_PERI_IRQ mask)
{
    REG_SYSTEM_0x20 |= (uint32_t)(mask & 0xFFFFFFFF);
    REG_SYSTEM_0x21 |= (uint32_t)(mask >> 32);
}

void system_peri_cpu1_irq_enable(SYS_PERI_IRQ mask)
{
    REG_SYSTEM_0x22 |= (uint32_t)(mask & 0xFFFFFFFF);
    REG_SYSTEM_0x23 |= (uint32_t)(mask >> 32);
}

void system_peri_cpu0_irq_disable(SYS_PERI_IRQ mask)
{
    REG_SYSTEM_0x20 &= ~(uint32_t)(mask & 0xFFFFFFFF);
    REG_SYSTEM_0x21 &= ~(uint32_t)(mask >> 32);
}

void system_peri_cpu1_irq_disable(SYS_PERI_IRQ mask)
{
    REG_SYSTEM_0x22 &= ~(uint32_t)(mask & 0xFFFFFFFF);
    REG_SYSTEM_0x23 &= ~(uint32_t)(mask >> 32);
}

void system_peri_cpu0_halt(void)
{
    REG_SYSTEM_0x04 |= (1 << 3); __asm("wfi");
}

void system_peri_cpu1_halt(void)
{
    REG_SYSTEM_0x05 |= (1 << 3);
}

void system_gpio_peri_config(uint32_t idx, uint32_t mode)
{
    uint32_t ofs = idx * 4 / 32;
    uint32_t pos = idx * 4 - ofs * 32;

    *(volatile uint32_t*)(&REG_SYSTEM_0x30 + ofs) = (*(volatile uint32_t*)(&REG_SYSTEM_0x30 + ofs) & ~(0xF << pos)) | (mode << pos);
}

int32_t system_ctrl(uint32_t cmd, uint32_t arg)
{
    int32_t  res = 0;

    switch(cmd)
    {
    default:
        res = -1;
        break;
    }

    return res;
}

void fpga_sys_drv_early_init(void)
{
    REG_WDT_CONFIG = 0x5A0000;
	REG_WDT_CONFIG = 0xA50000;
	REG_AONWDT_CONFIG = 0x5A0000;
	REG_AONWDT_CONFIG = 0xA50000;

    gpio_config(GPIO10, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
    gpio_config(GPIO11, GPIO_INPUT,  GPIO_PULL_UP, GPIO_PERI_FUNC1);
    system_peri_clk_enable(SYS_PERI_CLK_UART0);
    system_peri_cpu0_irq_enable(SYS_PERI_IRQ_UART0);

    uart_init(PRINT_UART_PORT, 115200, UART_DATA_BITS_8, UART_STOP_BITS_1, UART_PARITY_NONE);

    bk_printf("Welcome to use BEKEN's chip BK7286!\n");
    bk_printf("Firmware build @ %s %s\n", __TIME__, __DATE__);

    gpio_config(GPIO20, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC2);//SWC
    gpio_config(GPIO21, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC2);//SWD
}

