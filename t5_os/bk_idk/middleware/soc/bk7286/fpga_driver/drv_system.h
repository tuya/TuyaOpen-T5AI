#ifndef __DRV_SYSTEM_H__
#define __DRV_SYSTEM_H__

#include <stdint.h>

#define CHIP_ID    7286

/**
 * @brief System clock for peripherals definition
 */
typedef enum
{
    SYS_PERI_CLK_SLCD   = (1ULL << (32 + 4)),
    SYS_PERI_CLK_YUV    = (1ULL << (32 + 3)),
    SYS_PERI_CLK_I2S2   = (1ULL << (32 + 2)),
    SYS_PERI_CLK_I2S1   = (1ULL << (32 + 1)),
    SYS_PERI_CLK_H264   = (1ULL << (32 + 0)),
    SYS_PERI_CLK_WDT    = (1ULL << 31),
    SYS_PERI_CLK_AUD    = (1ULL << 30),
    SYS_PERI_CLK_DISP   = (1ULL << 29),
    SYS_PERI_CLK_JPEG   = (1ULL << 28),
    SYS_PERI_CLK_PHY    = (1ULL << 27),
    SYS_PERI_CLK_MAC    = (1ULL << 26),
    SYS_PERI_CLK_XVR    = (1ULL << 25),
    SYS_PERI_CLK_BTDM   = (1ULL << 24),
    SYS_PERI_CLK_AUXS   = (1ULL << 23),
    SYS_PERI_CLK_SDIO   = (1ULL << 22),
    SYS_PERI_CLK_QSPI1  = (1ULL << 21),
    SYS_PERI_CLK_QSPI0  = (1ULL << 20),
    SYS_PERI_CLK_PSRAM  = (1ULL << 19),
    SYS_PERI_CLK_CAN    = (1ULL << 18),
    SYS_PERI_CLK_USB    = (1ULL << 17),
    SYS_PERI_CLK_I2S    = (1ULL << 16),
    SYS_PERI_CLK_OTP    = (1ULL << 15),
    SYS_PERI_CLK_TIM2   = (1ULL << 14),
    SYS_PERI_CLK_TIM1   = (1ULL << 13),
    SYS_PERI_CLK_PWM1   = (1ULL << 12),
    SYS_PERI_CLK_UART2  = (1ULL << 11),
    SYS_PERI_CLK_UART1  = (1ULL << 10),
    SYS_PERI_CLK_SPI1   = (1ULL <<  9),
    SYS_PERI_CLK_I2C1   = (1ULL <<  8),
    SYS_PERI_CLK_EFUSE  = (1ULL <<  7),
    SYS_PERI_CLK_IRDA   = (1ULL <<  6),
    SYS_PERI_CLK_SADC   = (1ULL <<  5),
    SYS_PERI_CLK_TIM0   = (1ULL <<  4),
    SYS_PERI_CLK_PWM0   = (1ULL <<  3),
    SYS_PERI_CLK_UART0  = (1ULL <<  2),
    SYS_PERI_CLK_SPI0   = (1ULL <<  1),
    SYS_PERI_CLK_I2C0   = (1ULL <<  0),
}SYS_PERI_CLK;

/**
 * @brief System peripherals IRQ definition
 */
typedef enum
{
    SYS_PERI_IRQ_WDT            = (1ULL << 32),
    SYS_PERI_IRQ_TIMR           = (1ULL << 31),
    SYS_PERI_IRQ_UART0          = (1ULL << 30),
    SYS_PERI_IRQ_SPI0           = (1ULL << 29),
    SYS_PERI_IRQ_DMA0_NONSEC    = (1ULL << 28),
    SYS_PERI_IRQ_DMA0_SEC       = (1ULL << 27),
    SYS_PERI_IRQ_DMA1_NONSEC    = (1ULL << 26),
    SYS_PERI_IRQ_DMA1_SEC       = (1ULL << 25),
    SYS_PERI_IRQ_LA             = (1ULL << 24),
    SYS_PERI_IRQ_TIMER1         = (1ULL << 23),
    SYS_PERI_IRQ_SDMADC         = (1ULL << 22),
    SYS_PERI_IRQ_UART1          = (1ULL << 21),
    SYS_PERI_IRQ_UART2          = (1ULL << 20),
    SYS_PERI_IRQ_I2C0           = (1ULL << 19),
    SYS_PERI_IRQ_I2C1           = (1ULL << 18),
    SYS_PERI_IRQ_SPI1           = (1ULL << 17),
    SYS_PERI_IRQ_SADC           = (1ULL << 16),
    SYS_PERI_IRQ_PWM            = (1ULL << 15),
    SYS_PERI_IRQ_IRDA           = (1ULL << 14),
    SYS_PERI_IRQ_SDIO           = (1ULL << 13),
    SYS_PERI_IRQ_PWM1           = (1ULL << 12),
    SYS_PERI_IRQ_USB            = (1ULL << 11),
    SYS_PERI_IRQ_CAN            = (1ULL << 10),
    SYS_PERI_IRQ_QSPI0          = (1ULL <<  9),
    SYS_PERI_IRQ_QSPI1          = (1ULL <<  8),
    SYS_PERI_IRQ_INT_GPIO       = (1ULL <<  7),
    SYS_PERI_IRQ_DPU            = (1ULL <<  6),
    SYS_PERI_IRQ_XAQ2_INTR      = (1ULL <<  5),
    SYS_PERI_IRQ_ISP_MI         = (1ULL <<  4),
    SYS_PERI_IRQ_ISP_FE         = (1ULL <<  3),
    SYS_PERI_IRQ_ISP_ISP        = (1ULL <<  2),
    SYS_PERI_IRQ_H26D           = (1ULL <<  1),
    SYS_PERI_IRQ_H26E           = (1ULL <<  0),
}SYS_PERI_IRQ;

/**
 * @brief System delay for n cycles
 * @param n delay cycles
 */
void sys_delay_cycle(uint32_t n);

/**
 * @brief System delay for us in micro-second
 * @param us delay time in micro-second
 */
void sys_delay_us(uint32_t us);

/**
 * @brief System delay for ms in milli-second
 * @param ms delay time in milli-second
 */
void sys_delay_ms(uint32_t ms);

/**
 * @brief System initialization
 */
void system_init(void);

/**
 * @brief System register read operation
 * @param index register index
 * @param value register value
 */
void system_reg_write(uint32_t index, uint32_t value);

/**
 * @brief System peripheral clock enable
 * @param peries peripheral, @see SYS_PERI_CLK
 */
void system_peri_clk_enable(SYS_PERI_CLK peries);

/**
 * @brief System peripheral clock disable
 * @param peries peripheral, @see SYS_PERI_CLK
 */
void system_peri_clk_disable(SYS_PERI_CLK peries);

/**
 * @brief System peripheral interrupt enable for CPU0/1
 * @param peries peripheral interrupt mask, @see SYS_PERI_IRQ
 */
void system_peri_cpu0_irq_enable(SYS_PERI_IRQ peries);
void system_peri_cpu1_irq_enable(SYS_PERI_IRQ peries);

/**
 * @brief System peripheral interrupt disable for CPU0/1
 * @param mask peripheral interrupt mask, @see SYS_PERI_IRQ
 */
void system_peri_cpu0_irq_disable(SYS_PERI_IRQ peries);
void system_peri_cpu1_irq_disable(SYS_PERI_IRQ peries);

void system_peri_cpu0_halt(void);
void system_peri_cpu1_halt(void);

/**
 * @brief System GPIO mapping to peripheral (function) mode configuration
 * @param idx  GPIO index, range from 0 to 39, @see GPIO_PIN
 * @param mode peripheral (function) mode, range from 0 to 3
 */
void system_gpio_peri_config(uint32_t idx, uint32_t mode);

/**
 * @brief System control
 * @param cmd control command, @see SYS_CTRL_CMD
 * @param arg control argument
 * @return control result, 0:SUCCESS, others:FAILURE
 */
int32_t system_ctrl(uint32_t cmd, uint32_t arg);

#endif//__DRV_SYSTEM_H__
