#include "tuya_dma.h"
#include "tkl_spi.h"
#include "tkl_qspi.h"
#include <driver/spi.h>
#include <sdkconfig.h>
#include "qspi_hal.h"

#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/spi.h>
#include "clock_driver.h"
#include "dma_hal.h"
#include "gpio_driver.h"
#include "icu_driver.h"
#include <common/bk_include.h>
#include <os/mem.h>
#include "power_driver.h"
#include <os/os.h>
#include "spi_driver.h"
#include "spi_hal.h"
#include "spi_statis.h"
#include "spi_config.h"
#include "sys_driver.h"
#include "bk_misc.h"
#if CONFIG_SPE
#include "security.h"
#endif
#if CONFIG_SPI_PM_CB_SUPPORT
#include <modules/pm.h>
#endif

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t tkl_spi_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP

#define SPI_DMA_MAX_TRANSFER_LEN 65535
#define SPI_DMA_MAX_LEN 0xffffffff
#define SOC_SPI_MAX_NUM  (2)

#define TKL_SPI_INT_CONFIG_TABLE \
{\
	{SPI_ID_0, INT_SRC_SPI0, tkl_spi_isr, DMA_DEV_GSPI0},\
	{SPI_ID_1, INT_SRC_SPI1, tkl_spi2_isr, DMA_DEV_GSPI1},\
}

#define TKL_SPI_RX_INT_CONFIG_TABLE \
{\
	{SPI_ID_0, INT_SRC_SPI0, tkl_spi_isr, DMA_DEV_GSPI0_RX},\
	{SPI_ID_1, INT_SRC_SPI1, tkl_spi2_isr, DMA_DEV_GSPI1_RX},\
}

static spi_config_t spi_config[TUYA_SPI_NUM_MAX] = {0};

struct spi_irq_config {
    uint8_t irq_enable;
    TUYA_SPI_IRQ_CB cb;
};
static struct spi_irq_config spi_irq[TUYA_SPI_NUM_MAX] = {0};

#if (CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS_SMP && CONFIG_DCACHE)
/* Redmine #8131: SPI DMA shares a bare physical buffer with the DMA engine while
 * the AP cores run private, non-coherent D-caches and task stacks live in the
 * Cacheable PSRAM stack heap. For a DMA-from-memory (TX) transfer the source is
 * cleaned to physical PSRAM before the engine reads it - clean+invalidate is safe
 * even on a partially-owned cache line because the dirty data is written back
 * first. For a DMA-to-memory (RX) transfer a post-completion invalidate would
 * discard live neighbouring stack data on a shared line, so a Cacheable-stack RX
 * buffer is bounced through a non-cacheable heap buffer instead. Buffers outside
 * the stack heap (frame/heap buffers) are untouched (no-op). */
extern void flush_dcache(void *va, long size);
#define SPI_DCACHE_LINE_SIZE 32U

static inline int spi_addr_in_cacheable_stack(const void *p)
{
    uint32_t a = (uint32_t)p;
    return (p != NULL) &&
           (a >= (uint32_t)CONFIG_AP_PSRAM_STACK_HEAP_ADDR) &&
           (a <  (uint32_t)(CONFIG_AP_PSRAM_STACK_HEAP_ADDR + CONFIG_AP_PSRAM_STACK_HEAP_SIZE));
}

static inline void spi_cacheable_stack_clean(const void *p, uint32_t len)
{
    if (spi_addr_in_cacheable_stack(p) && (len != 0U)) {
        uint32_t s = (uint32_t)p & ~(SPI_DCACHE_LINE_SIZE - 1U);
        uint32_t e = ((uint32_t)p + len + SPI_DCACHE_LINE_SIZE - 1U) &
                     ~(SPI_DCACHE_LINE_SIZE - 1U);
        flush_dcache((void *)s, (long)(e - s));
    }
}
#else
#define spi_addr_in_cacheable_stack(p)   (0)
#define spi_cacheable_stack_clean(p, l)  ((void)0)
#endif

typedef struct {
	spi_hal_t hal;
	uint8_t id_init_bits;
	uint8_t * tx_buf;
	uint32_t tx_size;
	uint8_t * rx_buf;
	uint32_t rx_size;
	volatile uint32_t rx_offset;
	bool is_tx_blocked;
	bool is_sw_tx_finished;
	bool is_rx_blocked;
	beken_semaphore_t tx_sema;
	beken_semaphore_t rx_sema;
	dma_id_t spi_tx_dma_chan;
	dma_id_t spi_rx_dma_chan;
	bool dma_inited;
#if CONFIG_SPI_PM_CB_SUPPORT
	uint32_t pm_backup[SPI_PM_BACKUP_REG_NUM];
	uint8_t pm_backup_is_valid;
#endif
} tkl_spi_driver_t;

typedef struct {
	spi_isr_t callback;
	void *param;
} tkl_spi_callback_t;

#define SPI_RETURN_NOT_INIT() do {\
	if (!spi_driver_is_init) {\
		bk_printf("SPI driver not init\r\n");\
		return BK_ERR_SPI_NOT_INIT;\
	}\
} while(0)

#define SPI_RETURN_ID_NOT_INIT(id) do {\
	if (!spi[id].id_init_bits) {\
		bk_printf("SPI(%d) not init\r\n", id);\
		return BK_ERR_SPI_ID_NOT_INIT;\
	}\
} while(0)

#define SPI_RETURN_INVALID_ID(id) do {\
	if ((id) >= SOC_SPI_MAX_NUM) {\
		bk_printf("SPI id number(%d) is invalid\r\n", (id));\
		return BK_ERR_SPI_INVALID_ID;\
	}\
} while(0)

#if CONFIG_SPE
#define SPI_CHECK_SECURE(id) do {\
	switch (id) {\
	case SPI_ID_0:\
		BK_ASSERT(DEV_IS_SECURE(SPI0) == 1);\
		break;\
	case SPI_ID_1:\
		BK_ASSERT(DEV_IS_SECURE(SPI1) == 1);\
		break;\
	default:\
		break;\
	}\
} while(0)
#else
#define SPI_CHECK_SECURE(id)
#endif


#define TKL_SPI0_G0_SCK_PIN     GPIO_14
#define TKL_SPI0_G0_CSN_PIN     GPIO_15
#define TKL_SPI0_G0_MOSI_PIN    GPIO_16
#define TKL_SPI0_G0_MISO_PIN    GPIO_17

#define TKL_SPI0_G1_SCK_PIN     GPIO_33
#define TKL_SPI0_G1_CSN_PIN     GPIO_34
#define TKL_SPI0_G1_MOSI_PIN    GPIO_35
#define TKL_SPI0_G1_MISO_PIN    GPIO_36

#define TKL_SPI0_G2_SCK_PIN     GPIO_44
#define TKL_SPI0_G2_CSN_PIN     GPIO_45
#define TKL_SPI0_G2_MOSI_PIN    GPIO_46
#define TKL_SPI0_G2_MISO_PIN    GPIO_47


#define TKL_SPI1_SCK_PIN        GPIO_2
#define TKL_SPI1_CSN_PIN        GPIO_3
#define TKL_SPI1_MOSI_PIN       GPIO_4
#define TKL_SPI1_MISO_PIN       GPIO_5

struct tkl_spi_pin_s {
	gpio_id_t sck;
	gpio_id_t csn;
	gpio_id_t mosi;
	gpio_id_t miso;
};
static struct tkl_spi_pin_s spi_pin_config[SOC_SPI_MAX_NUM] = {
	{
		.sck = TKL_SPI0_G0_SCK_PIN,
		.csn = TKL_SPI0_G0_CSN_PIN,
		.mosi = TKL_SPI0_G0_MOSI_PIN,
		.miso = TKL_SPI0_G0_MISO_PIN,
	},
	{
		.sck = TKL_SPI1_SCK_PIN,
		.csn = TKL_SPI1_CSN_PIN,
		.mosi = TKL_SPI1_MOSI_PIN,
		.miso = TKL_SPI1_MISO_PIN,
	},
};

/**
 * @brief which SPI port/role a pin can serve, for tkl_io_pin_to_func()
 *
 * @param[in] pin: gpio number
 * @return (port << 8) | role   with role 0 = MISO, 1 = MOSI, 2 = CLK, 3 = CS
 *         OPRT_NOT_SUPPORTED if the pin has no SPI function
 *
 * Only SPI0 has selectable pins on this chip (its three groups are the
 * TKL_SPI0_G* defines above); SPI1..3 are fixed and not reported here.
 */
int32_t __tkl_spi_pin_to_func(TUYA_PIN_NAME_E pin)
{
    if ((pin == TKL_SPI0_G0_MISO_PIN) || (pin == TKL_SPI0_G1_MISO_PIN) ||
        (pin == TKL_SPI0_G2_MISO_PIN)) {
        return (0 << 8) | 0;
    }
    if ((pin == TKL_SPI0_G0_MOSI_PIN) || (pin == TKL_SPI0_G1_MOSI_PIN) ||
        (pin == TKL_SPI0_G2_MOSI_PIN)) {
        return (0 << 8) | 1;
    }
    if ((pin == TKL_SPI0_G0_SCK_PIN) || (pin == TKL_SPI0_G1_SCK_PIN) ||
        (pin == TKL_SPI0_G2_SCK_PIN)) {
        return (0 << 8) | 2;
    }
    if ((pin == TKL_SPI0_G0_CSN_PIN) || (pin == TKL_SPI0_G1_CSN_PIN) ||
        (pin == TKL_SPI0_G2_CSN_PIN)) {
        return (0 << 8) | 3;
    }
    return OPRT_NOT_SUPPORTED;
}

void __tkl_spi_set_clk_pin(TUYA_SPI_NUM_E port, TUYA_PIN_NAME_E pin)
{
	// only support set spi0
	if (port == TUYA_SPI_NUM_0) {
		if (TUYA_IO_PIN_14 == pin) {
			spi_pin_config[0].sck = GPIO_14;
		} else if (TUYA_IO_PIN_33 == pin) {
			spi_pin_config[0].sck = GPIO_33;
		} else if (TUYA_IO_PIN_44 == pin) {
			spi_pin_config[0].sck = GPIO_44;
		}
	}
}

void __tkl_spi_set_cs_pin(TUYA_SPI_NUM_E port, TUYA_PIN_NAME_E pin)
{
	// only support set spi0
	if (port == TUYA_SPI_NUM_0) {
		if (TUYA_IO_PIN_15 == pin) {
			spi_pin_config[0].csn = GPIO_15;
		} else if (TUYA_IO_PIN_34 == pin) {
			spi_pin_config[0].csn = GPIO_34;
		} else if (TUYA_IO_PIN_45 == pin) {
			spi_pin_config[0].csn = GPIO_45;
		}
	}
}

void __tkl_spi_set_mosi_pin(TUYA_SPI_NUM_E port, TUYA_PIN_NAME_E pin)
{
	// only support set spi0
	if (port == TUYA_SPI_NUM_0) {
		if (TUYA_IO_PIN_16 == pin) {
			spi_pin_config[0].mosi = GPIO_16;
		} else if (TUYA_IO_PIN_35 == pin) {
			spi_pin_config[0].mosi = GPIO_35;
		} else if (TUYA_IO_PIN_46 == pin) {
			spi_pin_config[0].mosi = GPIO_46;
		}
	}
}

void __tkl_spi_set_miso_pin(TUYA_SPI_NUM_E port, TUYA_PIN_NAME_E pin)
{
	// only support set spi0
	if (port == TUYA_SPI_NUM_0) {
		if (TUYA_IO_PIN_17 == pin) {
			spi_pin_config[0].miso = GPIO_17;
		} else if (TUYA_IO_PIN_36 == pin) {
			spi_pin_config[0].miso = GPIO_36;
		} else if (TUYA_IO_PIN_47 == pin) {
			spi_pin_config[0].miso = GPIO_47;
		}
	}
}

static int __tkl_check_spi0_pins(void)
{
	static const struct tkl_spi_pin_s valid_pin_groups[] = {
		{TKL_SPI0_G0_SCK_PIN, TKL_SPI0_G0_CSN_PIN, TKL_SPI0_G0_MOSI_PIN, TKL_SPI0_G0_MISO_PIN},
		{TKL_SPI0_G1_SCK_PIN, TKL_SPI0_G1_CSN_PIN, TKL_SPI0_G1_MOSI_PIN, TKL_SPI0_G1_MISO_PIN},
		{TKL_SPI0_G2_SCK_PIN, TKL_SPI0_G2_CSN_PIN, TKL_SPI0_G2_MOSI_PIN, TKL_SPI0_G2_MISO_PIN},
	};

	for (int i = 0; i < sizeof(valid_pin_groups) / sizeof(valid_pin_groups[0]); i++) {
		if ((spi_pin_config[0].sck == valid_pin_groups[i].sck) &&
		    (spi_pin_config[0].csn == valid_pin_groups[i].csn) &&
		    (spi_pin_config[0].mosi == valid_pin_groups[i].mosi) &&
		    (spi_pin_config[0].miso == valid_pin_groups[i].miso)) {
			return BK_OK;
		}
	}

	return BK_FAIL;
}

static gpio_id_t __tkl_spi_get_pin(TUYA_SPI_NUM_E port, gpio_id_t default_pin,
                                    gpio_id_t (*get_config_pin)(uint8_t))
{
	if (port > 1)
		return 56;

	if (port == 0 ) {
		return (__tkl_check_spi0_pins() == BK_OK) ? get_config_pin(0) : default_pin;
	}
	return get_config_pin(port);
}

static gpio_id_t get_sck_pin(uint8_t port) { return spi_pin_config[port].sck; }
static gpio_id_t get_csn_pin(uint8_t port) { return spi_pin_config[port].csn; }
static gpio_id_t get_mosi_pin(uint8_t port) { return spi_pin_config[port].mosi; }
static gpio_id_t get_miso_pin(uint8_t port) { return spi_pin_config[port].miso; }

gpio_id_t __tkl_spi_get_cs_pin(TUYA_SPI_NUM_E port)
{
	return __tkl_spi_get_pin(port, TKL_SPI0_G0_CSN_PIN, get_csn_pin);
}

gpio_id_t __tkl_spi_get_clk_pin(TUYA_SPI_NUM_E port)
{
	return __tkl_spi_get_pin(port, TKL_SPI0_G0_SCK_PIN, get_sck_pin);
}

gpio_id_t __tkl_spi_get_mosi_pin(TUYA_SPI_NUM_E port)
{
	return __tkl_spi_get_pin(port, TKL_SPI0_G0_MOSI_PIN, get_mosi_pin);
}

gpio_id_t __tkl_spi_get_miso_pin(TUYA_SPI_NUM_E port)
{
	return __tkl_spi_get_pin(port, TKL_SPI0_G0_MISO_PIN, get_miso_pin);
}

#define TKL_SPI_SET_PIN(id) do {\
	gpio_dev_unmap(__tkl_spi_get_cs_pin(id));\
	gpio_dev_unmap(__tkl_spi_get_clk_pin(id));\
	gpio_dev_unmap(__tkl_spi_get_mosi_pin(id));\
	gpio_dev_unmap(__tkl_spi_get_miso_pin(id));\
	gpio_dev_map(__tkl_spi_get_cs_pin(id), GPIO_DEV_SPI##id##_CSN);\
	gpio_dev_map(__tkl_spi_get_clk_pin(id), GPIO_DEV_SPI##id##_SCK);\
	gpio_dev_map(__tkl_spi_get_mosi_pin(id), GPIO_DEV_SPI##id##_MOSI);\
	gpio_dev_map(__tkl_spi_get_miso_pin(id), GPIO_DEV_SPI##id##_MISO);\
	bk_gpio_pull_up(__tkl_spi_get_cs_pin(id));\
	bk_gpio_pull_up(__tkl_spi_get_clk_pin(id));\
	bk_gpio_set_capacity(__tkl_spi_get_cs_pin(id), 0);\
	bk_gpio_set_capacity(__tkl_spi_get_clk_pin(id), 1);\
	bk_gpio_set_capacity(__tkl_spi_get_mosi_pin(id), 0);\
	bk_gpio_set_capacity(__tkl_spi_get_miso_pin(id), 0);\
} while(0)

static tkl_spi_driver_t spi[SOC_SPI_MAX_NUM] = {
	{
		.hal.hw = (spi_hw_t *)(SOC_SPI_REG_BASE),
		.spi_rx_dma_chan = DMA_ID_MAX,
		.spi_tx_dma_chan = DMA_ID_MAX
	},
#if (SOC_SPI_MAX_NUM > 1)
	{
		.hal.hw = (spi_hw_t *)(SOC_SPI1_REG_BASE),
		.spi_rx_dma_chan = DMA_ID_MAX,
		.spi_tx_dma_chan = DMA_ID_MAX
	}
#endif
};
static bool spi_driver_is_init = false;
static tkl_spi_callback_t spi_rx_isr[SOC_SPI_MAX_NUM] = {NULL};
static tkl_spi_callback_t spi_tx_finish_isr[SOC_SPI_MAX_NUM] = {NULL};
static tkl_spi_callback_t spi_rx_finish_isr[SOC_SPI_MAX_NUM] = {NULL};

static void tkl_spi_isr(void);
#if (SOC_SPI_MAX_NUM > 1)
static void tkl_spi2_isr(void);
#endif
#if (SOC_SPI_MAX_NUM > 2)
static void tkl_spi3_isr(void);
#endif

static inline uint32_t spi_enter_critical()
{
       uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
       spin_lock(&tkl_spi_spin_lock);
#endif // CONFIG_FREERTOS_SMP

       return flags;
}

static inline void spi_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
       spin_unlock(&tkl_spi_spin_lock);
#endif // CONFIG_FREERTOS_SMP

       rtos_enable_int(flags);
}

static void spi_init_gpio(spi_id_t id)
{
	switch (id) {
	case SPI_ID_0:
		TKL_SPI_SET_PIN(0);
		break;
#if (SOC_SPI_MAX_NUM > 1)
	case SPI_ID_1:
		TKL_SPI_SET_PIN(1);
		break;
#endif
#if (SOC_SPI_MAX_NUM > 2)
	case SPI_ID_2:
		TKL_SPI_SET_PIN(2);
		break;
#endif
	default:
		break;
	}
#if (!CONFIG_SYSTEM_CTRL)
	gpio_spi_sel(GPIO_SPI_MAP_MODE1);
#endif
}

#if (CONFIG_SYSTEM_CTRL)
static void spi_clock_enable(spi_id_t id)
{
	switch(id)
	{
		case SPI_ID_0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_SPI_1, CLK_PWR_CTRL_PWR_UP);
			break;
#if (SOC_SPI_MAX_NUM > 1)
		case SPI_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_SPI_2, CLK_PWR_CTRL_PWR_UP);
			break;
#endif
		default:
			break;
	}
}

static void spi_clock_disable(spi_id_t id)
{
#if CONFIG_SOC_BK7256XX
	//disable apll clock, and restore the clock config
	sys_drv_apll_en(false);
	sys_drv_cb_manu_val_set(SPI_DIS_APLL_MANU_VAL);
	sys_drv_ana_reg11_vsel_set(SPI_DIS_APLL_VSEL_VAL);
#endif

	switch(id)
	{
		case SPI_ID_0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_SPI_1, CLK_PWR_CTRL_PWR_DOWN);
			break;
#if (SOC_SPI_MAX_NUM > 1)
		case SPI_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_SPI_2, CLK_PWR_CTRL_PWR_DOWN);
			break;
#endif
		default:
			break;
	}
}

static void spi_interrupt_enable(spi_id_t id)
{
	switch(id)
	{
		case SPI_ID_0:
			sys_drv_int_enable(SPI_INTERRUPT_CTRL_BIT);
			break;
#if (SOC_SPI_MAX_NUM > 1)
		case SPI_ID_1:
			sys_drv_int_enable(SPI1_INTERRUPT_CTRL_BIT);
			break;
#endif
		default:
			break;
	}
}

static void spi_interrupt_disable(spi_id_t id)
{
	switch(id)
	{
		case SPI_ID_0:
			sys_drv_int_disable(SPI_INTERRUPT_CTRL_BIT);
			break;
#if (SOC_SPI_MAX_NUM > 1)
		case SPI_ID_1:
			sys_drv_int_disable(SPI1_INTERRUPT_CTRL_BIT);
			break;
#endif
		default:
			break;
	}
}
#endif

/* 1. power up spi
 * 2. set clk
 * 3. set gpio as spi
 * 4. icu enable interrupt
 */
static OPERATE_RET spi_id_init_common(spi_id_t id)
{
	int ret = 0;

#if (CONFIG_SYSTEM_CTRL)
	spi_clock_enable(id);
	spi_interrupt_enable(id);
#else
	power_up_spi(id);
	clk_set_spi_clk_26m(id);
	icu_enable_spi_interrupt(id);
#endif
	spi_init_gpio(id);

	if (spi[id].tx_sema == NULL) {
		ret = rtos_init_semaphore(&(spi[id].tx_sema), 1);
		BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
	}
	if (spi[id].rx_sema == NULL) {
		ret = rtos_init_semaphore(&(spi[id].rx_sema), 1);
		BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
	}
	spi[id].is_tx_blocked = false;
	spi[id].is_sw_tx_finished = true;
	spi[id].is_rx_blocked = false;
	spi[id].id_init_bits |= BIT(id);

	return ret;
}

static void spi_id_deinit_common(spi_id_t id)
{
	spi_hal_stop_common(&spi[id].hal);

#if (CONFIG_SYSTEM_CTRL)
	spi_clock_disable(id);
	spi_interrupt_disable(id);
#else
	icu_disable_spi_interrupt(id);
	power_down_spi(id);
#endif
	if(spi[id].tx_sema){
		rtos_deinit_semaphore(&(spi[id].tx_sema));
	}

	if(spi[id].rx_sema){
		rtos_deinit_semaphore(&(spi[id].rx_sema));
	}
	spi[id].id_init_bits &= ~BIT(id);
}

static void spi_id_write_bytes_common(spi_id_t id)
{
	for (int i = 0; i < spi[id].tx_size; i++) {
		while (!spi_hal_is_tx_fifo_wr_ready(&spi[id].hal));
		spi_hal_write_byte(&spi[id].hal, spi[id].tx_buf[i]);
	}
}

static uint32_t spi_id_read_bytes_common(spi_id_t id)
{
	uint8_t data = 0;
	uint32_t offset = spi[id].rx_offset;

	while (spi_hal_read_byte(&spi[id].hal, &data) == OPRT_OK) {
		if ((spi[id].rx_buf) && (offset < spi[id].rx_size)) {
			spi[id].rx_buf[offset++] = data;
		}
	}
	spi[id].rx_offset = offset;
	SPI_LOGV("spi offset:%d\r\n", spi[id].rx_offset);
	return offset;
}

#if CONFIG_SPI_DMA

static void spi_dma_tx_finish_handler(dma_id_t id, DMA_ISR_TYPE_E event)
{
	int8_t spi_dma_wr_id = 0;
	for (size_t i = 0; i < SOC_SPI_MAX_NUM; i++) {
		if (spi[i].spi_tx_dma_chan == id) {
			spi_dma_wr_id = i;
			break;
		}
	}

	uint32_t int_level = spi_enter_critical();
    //wait spi last fifo data transfer finish
	spi_hal_enable_tx_fifo_int(&spi[spi_dma_wr_id].hal);
    for (int i = 0; i <= 500; i++) {
        bk_delay_us(1);
        if(spi_hal_is_tx_fifo_int_triggered(&spi[spi_dma_wr_id].hal)) {
            bk_delay_us(1);
            break;
        }
        if(i == 500)
            SPI_LOGE("wait tx fifo empty timeout.\n");
    }
	tkl_dma_stop(spi[spi_dma_wr_id].spi_tx_dma_chan);
	spi_hal_disable_tx(&spi[spi_dma_wr_id].hal);
	spi_hal_disable_tx_fifo_int(&spi[spi_dma_wr_id].hal);
	spi_hal_clear_tx_fifo_int_status(&spi[spi_dma_wr_id].hal);
	spi_exit_critical(int_level);

	if (spi[spi_dma_wr_id].is_tx_blocked) {
		rtos_set_semaphore(&spi[spi_dma_wr_id].tx_sema);
		spi[spi_dma_wr_id].is_tx_blocked = false;
	}

	if (spi_tx_finish_isr[spi_dma_wr_id].callback) {
		spi_tx_finish_isr[spi_dma_wr_id].callback(spi_dma_wr_id,spi_tx_finish_isr[spi_dma_wr_id].param);
	}
}

static void spi_dma_rx_finish_handler(dma_id_t id, DMA_ISR_TYPE_E event)
{
	int8_t spi_dma_rd_id = 0;
	for (size_t i = 0; i < SOC_SPI_MAX_NUM; i++) {
		if (spi[i].spi_rx_dma_chan == id) {
			spi_dma_rd_id = i;
			break;
		}
	}

	uint32_t int_level = spi_enter_critical();
	tkl_dma_stop(spi[spi_dma_rd_id].spi_rx_dma_chan);
	spi_hal_disable_rx(&spi[spi_dma_rd_id].hal);
	spi_hal_disable_rx_fifo_int(&spi[spi_dma_rd_id].hal);
	spi_hal_clear_rx_fifo_int_status(&spi[spi_dma_rd_id].hal);
	spi_exit_critical(int_level);

	if (spi_rx_finish_isr[spi_dma_rd_id].callback){
		spi_rx_finish_isr[spi_dma_rd_id].callback(spi_dma_rd_id,spi_rx_finish_isr[spi_dma_rd_id].param);
	}
	if (spi[spi_dma_rd_id].is_rx_blocked) {
		rtos_set_semaphore(&spi[spi_dma_rd_id].rx_sema);
		spi[spi_dma_rd_id].is_rx_blocked = false;
	}
}

static void spi_dma_tx_init(spi_id_t id, dma_id_t *spi_tx_dma_chan, TKL_DATA_WIDTH_T spi_tx_dma_width, TKL_WORK_MODE_E mode)
{
	TKL_DMA_CONFIG_T dma_config = {0};
	spi_int_config_t int_cfg_table[] = TKL_SPI_INT_CONFIG_TABLE;

	dma_config.mode = mode;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.width = spi_tx_dma_width;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_DISABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
	dma_config.dst.start_addr = SPI_R_DATA(id);
	dma_config.dst.dev = int_cfg_table[id].dma_dev;
	if (id == TUYA_SPI_NUM_0) {
    	dma_config.dev_id = DMA_DEV_GSPI0;
	}
	if (id == TUYA_SPI_NUM_1) {
    	dma_config.dev_id = DMA_DEV_GSPI1;
	}

	BK_LOG_ON_ERR(tkl_dma_init(spi_tx_dma_chan, &dma_config));
	BK_LOG_ON_ERR(tkl_dma_register_isr(*spi_tx_dma_chan, spi_dma_tx_finish_handler));
}

static void spi_dma_rx_init(spi_id_t id, dma_id_t *spi_rx_dma_chan, dma_data_width_t spi_rx_dma_width, dma_work_mode_t mode)
{
	TKL_DMA_CONFIG_T dma_config = {0};
	spi_int_config_t int_cfg_table[] = TKL_SPI_RX_INT_CONFIG_TABLE;

	dma_config.mode = mode;
	dma_config.src.dev = int_cfg_table[id].dma_dev;
	dma_config.src.width = spi_rx_dma_width;
	dma_config.src.start_addr = SPI_R_DATA(id);
	dma_config.src.addr_inc_en = DMA_ADDR_INC_DISABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
	if (id == TUYA_SPI_NUM_0) {
    	dma_config.dev_id = DMA_DEV_GSPI0_RX;
	}
	if (id == TUYA_SPI_NUM_1) {
    	dma_config.dev_id = DMA_DEV_GSPI1_RX;
	}

	BK_LOG_ON_ERR(tkl_dma_init(spi_rx_dma_chan, &dma_config));
	BK_LOG_ON_ERR(tkl_dma_register_isr(*spi_rx_dma_chan, spi_dma_rx_finish_handler));
}

#endif /* CONFIG_SPI_DMA */

#if (CONFIG_SPI_PM_CB_SUPPORT)
#define TKL_SPI_PM_CHECK_RESTORE(id) do {\
	if ((id == SPI_ID_1) && bk_pm_module_lv_sleep_state_get(PM_DEV_ID_SPI_2)) {\
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_SPI1, PM_POWER_MODULE_STATE_ON);\
		spi_restore(0, (void *)id);\
		bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_SPI_2); \
	}\
} while(0)

static int spi_backup(uint64_t sleep_time, void *args)
{
	spi_id_t id = (spi_id_t)args;
	SPI_RETURN_NOT_INIT();
	// SPI_RETURN_ID_NOT_INIT(id);
	if (!spi[id].pm_backup_is_valid)
	{
		spi[id].pm_backup_is_valid = 1;
		spi_hal_backup(&spi[id].hal, &spi[id].pm_backup[0]);
		spi_clock_disable(id);
	}
	return OPRT_OK;
}
static int spi_restore(uint64_t sleep_time, void *args)
{
	spi_id_t id = (spi_id_t)args;
	SPI_RETURN_NOT_INIT();
	//SPI_RETURN_ID_NOT_INIT(id);
	if (spi[id].pm_backup_is_valid)
	{
		spi_clock_enable(id);
		spi_hal_restore(&spi[id].hal, &spi[id].pm_backup[0]);
		spi[id].pm_backup_is_valid = 0;
	}
	return OPRT_OK;
}
#else
#define TKL_SPI_PM_CHECK_RESTORE(id)
#endif

static OPERATE_RET spi_driver_init(void)
{
	if (spi_driver_is_init) {
		return OPRT_OK;
	}

	spi_int_config_t int_config_table[] = TKL_SPI_INT_CONFIG_TABLE;
	os_memset(&spi_rx_isr, 0, sizeof(spi_rx_isr));
	os_memset(&spi_tx_finish_isr, 0, sizeof(spi_tx_finish_isr));
	for (int id = SPI_ID_0; id < SPI_ID_MAX; id++) {
#if (CONFIG_SPI_PM_CB_SUPPORT)
		if (id == SPI_ID_0) {
			//SPI_ID_0 of BK7236 is in AON domain
		} else if (id == SPI_ID_1) {
			bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_SPI1, PM_POWER_MODULE_STATE_ON);
		}
#endif
		spi_int_config_t *cur_int_cfg = &int_config_table[id];
		bk_int_isr_register(cur_int_cfg->int_src, cur_int_cfg->isr, NULL);
		spi[id].hal.id = id;
	}
	spi_driver_is_init = true;

	return OPRT_OK;
}

static OPERATE_RET spi_driver_deinit(void)
{
	if (!spi_driver_is_init) {
		return OPRT_OK;
	}
	spi_int_config_t int_cfg_table[] = TKL_SPI_INT_CONFIG_TABLE;
	for (int id = SPI_ID_0; id < SPI_ID_MAX; id++) {
		spi_id_deinit_common(id);
		bk_int_isr_unregister(int_cfg_table[id].int_src);
#if (CONFIG_SPI_PM_CB_SUPPORT)
		if (id == SPI_ID_0) {
			//SPI_ID_0 of BK7236 is in AON domain
		} else if (id == SPI_ID_1) {
			bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_SPI1, PM_POWER_MODULE_STATE_OFF);
		}
#endif
	}

	spi_driver_is_init = false;

	return OPRT_OK;
}

static OPERATE_RET spi_init(spi_id_t id, const spi_config_t *config)
{
	BK_RETURN_ON_NULL(config);
	SPI_RETURN_INVALID_ID(id);
	SPI_CHECK_SECURE(id);

#if (CONFIG_SPI_PM_CB_SUPPORT)
	pm_cb_conf_t enter_config = {spi_backup, (void *)id};
	if (id == SPI_ID_1) {
		bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_SPI_2);
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_SPI1, PM_POWER_MODULE_STATE_ON);
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_SPI_2, &enter_config, NULL);
	}
#endif
	spi_id_init_common(id);
	spi_hal_init(&spi[id].hal);
	spi_hal_configure(&spi[id].hal, config);
	spi_hal_start_common(&spi[id].hal);
#if (CONFIG_SPI_DMA)
	if (!spi[id].dma_inited) {
		if (config->dma_mode) {
#if (!CONFIG_SYSTEM_CTRL)
			gpio_spi_sel(GPIO_SPI_MAP_MODE0);
#endif
			spi_dma_tx_init(id, &spi[id].spi_tx_dma_chan, config->spi_tx_dma_width, DMA_WORK_MODE_SINGLE);
			spi_dma_rx_init(id, &spi[id].spi_rx_dma_chan, config->spi_rx_dma_width, DMA_WORK_MODE_SINGLE);
			//dma模式关闭外设中断，使用dma完成中断
			spi_interrupt_disable(id);
		}

		spi[id].dma_inited = true;
	}
#endif

	return OPRT_OK;
}

static OPERATE_RET spi_deinit(spi_id_t id)
{
	SPI_RETURN_INVALID_ID(id);

#if CONFIG_SPI_DMA
    if (spi[id].dma_inited) {
        if (spi[id].spi_tx_dma_chan != DMA_ID_MAX) {
            BK_LOG_ON_ERR(tkl_dma_deinit(spi[id].spi_tx_dma_chan));
            spi[id].spi_tx_dma_chan = DMA_ID_MAX;
        }
        if (spi[id].spi_rx_dma_chan != DMA_ID_MAX) {
            BK_LOG_ON_ERR(tkl_dma_deinit(spi[id].spi_rx_dma_chan));
            spi[id].spi_rx_dma_chan = DMA_ID_MAX;
        }
        spi[id].dma_inited = false;
    }
#endif

	spi_id_deinit_common(id);
#if (CONFIG_SPI_PM_CB_SUPPORT)
	if (id == SPI_ID_1) {
		bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_SPI_2, true, true);
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_SPI1, PM_POWER_MODULE_STATE_OFF);
	}
#endif

	return OPRT_OK;
}

static OPERATE_RET spi_register_rx_isr(spi_id_t id, spi_isr_t isr, void *param)
{
	SPI_RETURN_INVALID_ID(id);
	TKL_SPI_PM_CHECK_RESTORE(id);
	uint32_t int_level = spi_enter_critical();
	spi_rx_isr[id].callback = isr;
	spi_rx_isr[id].param = param;
	spi_exit_critical(int_level);
	return OPRT_OK;
}

static OPERATE_RET spi_register_rx_finish_isr(spi_id_t id, spi_isr_t isr, void *param)
{
	SPI_RETURN_INVALID_ID(id);
	TKL_SPI_PM_CHECK_RESTORE(id);
	uint32_t int_level = spi_enter_critical();
	spi_rx_finish_isr[id].callback = isr;
	spi_rx_finish_isr[id].param = param;
	spi_exit_critical(int_level);
	return OPRT_OK;
}


static OPERATE_RET spi_register_tx_finish_isr(spi_id_t id, spi_isr_t isr, void *param)
{
	SPI_RETURN_INVALID_ID(id);
	TKL_SPI_PM_CHECK_RESTORE(id);
	uint32_t int_level = spi_enter_critical();
	spi_tx_finish_isr[id].callback = isr;
	spi_tx_finish_isr[id].param = param;
	spi_exit_critical(int_level);
	return OPRT_OK;
}

static OPERATE_RET spi_unregister_rx_isr(spi_id_t id)
{
	SPI_RETURN_INVALID_ID(id);
	TKL_SPI_PM_CHECK_RESTORE(id);
	uint32_t int_level = spi_enter_critical();
	spi_rx_isr[id].callback = NULL;
	spi_rx_isr[id].param = NULL;
	spi_exit_critical(int_level);
	return OPRT_OK;
}

static OPERATE_RET spi_unregister_rx_finish_isr(spi_id_t id)
{
	SPI_RETURN_INVALID_ID(id);
	TKL_SPI_PM_CHECK_RESTORE(id);
	uint32_t int_level = spi_enter_critical();
	spi_rx_finish_isr[id].callback = NULL;
	spi_rx_finish_isr[id].param = NULL;
	spi_exit_critical(int_level);
	return OPRT_OK;
}


static OPERATE_RET spi_unregister_tx_finish_isr(spi_id_t id)
{
	SPI_RETURN_INVALID_ID(id);
	TKL_SPI_PM_CHECK_RESTORE(id);
	uint32_t int_level = spi_enter_critical();
	spi_tx_finish_isr[id].callback = NULL;
	spi_tx_finish_isr[id].param = NULL;
	spi_exit_critical(int_level);
	return OPRT_OK;
}


static OPERATE_RET spi_write_bytes(spi_id_t id, const void *data, uint32_t size)
{
	SPI_RETURN_NOT_INIT();
	SPI_RETURN_INVALID_ID(id);
	SPI_RETURN_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);
	TKL_SPI_PM_CHECK_RESTORE(id);

	uint32_t int_level = spi_enter_critical();
	spi[id].tx_buf = (uint8_t *)data;
	spi[id].tx_size = size;
	spi[id].is_sw_tx_finished = false;
	spi[id].is_tx_blocked = true;
	spi[id].rx_size = size;
	spi[id].rx_offset = 0;
	spi_hal_clear_tx_fifo(&spi[id].hal);
	spi_hal_set_tx_trans_len(&spi[id].hal, size);
#if CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	spi_hal_disable_tx_fifo_int(&spi[id].hal);
#else
	spi_hal_enable_tx_fifo_int(&spi[id].hal);
#endif
	spi_hal_enable_tx(&spi[id].hal);
	spi_exit_critical(int_level);

#if CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	spi[id].is_sw_tx_finished = true;
	spi_id_write_bytes_common(id); /* to solve slave write */
#endif

	rtos_get_semaphore(&spi[id].tx_sema, BEKEN_NEVER_TIMEOUT);

	int_level = spi_enter_critical();
	spi_hal_disable_tx_fifo_int(&spi[id].hal);
	spi_hal_disable_tx(&spi[id].hal);
	spi_exit_critical(int_level);

	return OPRT_OK;
}

static OPERATE_RET spi_read_bytes(spi_id_t id, void *data, uint32_t size)
{
	SPI_RETURN_NOT_INIT();
	SPI_RETURN_INVALID_ID(id);
	SPI_RETURN_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);
	TKL_SPI_PM_CHECK_RESTORE(id);

	uint32_t int_level = spi_enter_critical();
	spi[id].rx_size = size;
	spi[id].rx_buf = (uint8_t *)data;
	spi[id].rx_offset = 0;
	spi[id].is_rx_blocked = true;
	spi_hal_set_rx_trans_len(&spi[id].hal, size);
	spi_hal_clear_rx_fifo(&spi[id].hal);
	spi_hal_enable_rx_fifo_int(&spi[id].hal);
	spi_hal_enable_rx_finish_int(&spi[id].hal);
	spi_hal_enable_rx(&spi[id].hal);
#if !CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	/* special for bk7251, bk7251 need enable tx fifo int, otherwize tkl_spi_isr will not triggered */
	spi_hal_enable_tx_fifo_int(&spi[id].hal);
#endif
	spi_exit_critical(int_level);

	rtos_get_semaphore(&(spi[id].rx_sema), BEKEN_NEVER_TIMEOUT);

	int_level = spi_enter_critical();
	spi_hal_disable_rx(&spi[id].hal);
	spi_hal_disable_tx_fifo_int(&spi[id].hal);
	spi_hal_disable_rx_fifo_int(&spi[id].hal);
	spi[id].rx_size = 0;
	spi[id].rx_offset = 0;
	spi[id].rx_buf = NULL;
	spi_exit_critical(int_level);

	return OPRT_OK;
}

static OPERATE_RET spi_clr_tx(spi_id_t id)
{
    spi_hal_disable_tx_fifo_int(&spi[id].hal);
    spi_hal_disable_tx(&spi[id].hal);
    return OPRT_OK;
}

static OPERATE_RET spi_clr_rx(spi_id_t id)
{
    spi_hal_disable_rx(&spi[id].hal);
    spi_hal_disable_tx_fifo_int(&spi[id].hal);
    spi_hal_disable_rx_fifo_int(&spi[id].hal);
    spi[id].rx_size = 0;
    spi[id].rx_offset = 0;
    spi[id].rx_buf = NULL;
    return OPRT_OK;
}


static OPERATE_RET spi_write_bytes_async(spi_id_t id, const void *data, uint32_t size)
{
	SPI_RETURN_NOT_INIT();
	SPI_RETURN_INVALID_ID(id);
	SPI_RETURN_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);
	TKL_SPI_PM_CHECK_RESTORE(id);

	uint32_t int_level = spi_enter_critical();
	spi[id].tx_buf = (uint8_t *)data;
	spi[id].tx_size = size;
	spi[id].is_sw_tx_finished = false;
	spi[id].is_tx_blocked = false;
	spi[id].rx_size = size;
	spi[id].rx_offset = 0;
	spi_hal_clear_tx_fifo(&spi[id].hal);
	spi_hal_set_tx_trans_len(&spi[id].hal, size);
#if CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	spi_hal_disable_tx_fifo_int(&spi[id].hal);
#else
	spi_hal_enable_tx_fifo_int(&spi[id].hal);
#endif
	spi_hal_enable_tx(&spi[id].hal);
	spi_exit_critical(int_level);

#if CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	spi[id].is_sw_tx_finished = true;
	spi_id_write_bytes_common(id); /* to solve slave write */
#endif

	return OPRT_OK;
}

static OPERATE_RET spi_read_bytes_async(spi_id_t id, void *data, uint32_t size)
{
	SPI_RETURN_NOT_INIT();
	SPI_RETURN_INVALID_ID(id);
	SPI_RETURN_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);

	uint32_t int_level = spi_enter_critical();
	spi[id].rx_size = size;
	spi[id].rx_buf = (uint8_t *)data;
	spi[id].rx_offset = 0;
	spi[id].is_rx_blocked = false;
	spi_hal_set_rx_trans_len(&spi[id].hal, size);
	spi_hal_clear_rx_fifo(&spi[id].hal);
	spi_hal_enable_rx_fifo_int(&spi[id].hal);
	spi_hal_enable_rx_finish_int(&spi[id].hal);
	spi_hal_enable_rx(&spi[id].hal);
#if !CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	/* special for bk7251, bk7251 need enable tx fifo int, otherwize tkl_spi_isr will not triggered */
	spi_hal_enable_tx_fifo_int(&spi[id].hal);
#endif
	spi_exit_critical(int_level);

	return OPRT_OK;
}


#if CONFIG_SPI_DMA


static OPERATE_RET spi_dma_duplex_init(spi_id_t id)
{
	spi_hal_duplex_config(&spi[id].hal);
	return OPRT_OK;
}

static OPERATE_RET spi_dma_duplex_deinit(spi_id_t id)
{
	spi_hal_duplex_release(&spi[id].hal);
	return OPRT_OK;
}

static OPERATE_RET spi_dma_write_bytes(spi_id_t id, const void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);
	SPI_RETURN_INVALID_ID(id);
	SPI_RETURN_ID_NOT_INIT(id);

	int32_t left_len = size;
	uint32_t tx_len = 0;
	uint32_t buf_offset = 0;
	uint32_t int_level = spi_enter_critical();
	spi[id].is_tx_blocked = true;
	spi_hal_clear_tx_fifo(&spi[id].hal);
	spi_hal_set_tx_trans_len(&spi[id].hal, 0);
	spi_hal_disable_tx_fifo_int(&spi[id].hal);
	spi_hal_disable_tx_underflow_int(&spi[id].hal);
	spi_exit_critical(int_level);
	/* Redmine #8131: clean a Cacheable PSRAM stack source before the DMA reads it. */
	spi_cacheable_stack_clean(data, size);
	tkl_dma_write(spi[id].spi_tx_dma_chan, (uint32_t)data, size);

	spi_hal_enable_tx(&spi[id].hal);
	rtos_get_semaphore(&spi[id].tx_sema, BEKEN_NEVER_TIMEOUT);
	int_level = spi_enter_critical();
	spi_hal_disable_tx(&spi[id].hal);
	tkl_dma_stop(spi[id].spi_tx_dma_chan);
	spi_exit_critical(int_level);
	return OPRT_OK;
}

static OPERATE_RET spi_dma_read_bytes(spi_id_t id, void *data, uint32_t size)
{
	SPI_RETURN_INVALID_ID(id);
	SPI_RETURN_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);

	int32_t left_len = size;
	uint32_t rx_len = 0;
	uint32_t buf_offset = 0;

	/* Redmine #8131: bounce a Cacheable PSRAM stack destination through a
	 * non-cacheable heap buffer so the post-DMA read never has to invalidate a
	 * cache line shared with live neighbouring stack data. */
	void *xfer = data;
	void *bounce = NULL;
	if (spi_addr_in_cacheable_stack(data) && (size != 0)) {
		bounce = os_malloc(size);
		if (bounce == NULL) {
			return OPRT_MALLOC_FAILED;
		}
		xfer = bounce;
	}
	uint32_t int_level = spi_enter_critical();
	spi[id].is_rx_blocked = true;
	//set spi trans_len as 0, to increase max trans_len from 4096(spi max length) to 65536(dma max length).
	spi_hal_set_rx_trans_len(&spi[id].hal, 4);
	spi_hal_clear_rx_fifo(&spi[id].hal);
	spi_hal_disable_rx_fifo_int(&spi[id].hal);
	spi_hal_disable_rx_overflow_int(&spi[id].hal);
	spi_exit_critical(int_level);
	tkl_dma_read(spi[id].spi_rx_dma_chan, (uint32_t)xfer, size);

	spi_hal_enable_rx(&spi[id].hal);
	rtos_get_semaphore(&spi[id].rx_sema, BEKEN_NEVER_TIMEOUT);
	int_level = spi_enter_critical();
	spi_hal_disable_rx(&spi[id].hal);
	tkl_dma_stop(spi[id].spi_rx_dma_chan);
	spi_exit_critical(int_level);

	if (bounce != NULL) {
		memcpy(data, bounce, size);
		os_free(bounce);
	}
	return OPRT_OK;
}
#endif

static void tkl_spi_isr_common(spi_id_t id)
{
	spi_hal_t *hal = &spi[id].hal;
	uint8_t rd_data = 0;
	uint32_t int_status = 0;
	uint32_t rd_offset = spi[id].rx_offset;
	SPI_STATIS_DEC();
	SPI_STATIS_GET(spi_statis, id);

	int_status = spi_hal_get_interrupt_status(hal);
	spi_hal_clear_interrupt_status(hal, int_status);

	SPI_LOGV("int_status:%x\r\n", int_status);

	if (spi_hal_is_rx_fifo_int_triggered_with_status(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->rx_fifo_isr_cnt);
		SPI_LOGV("rx fifo int triggered\r\n");
		spi_id_read_bytes_common(id);
		if (spi_rx_isr[id].callback) {
			spi_rx_isr[id].callback(id, spi_rx_isr[id].param);
		}
	}

	if (spi_hal_is_rx_finish_int_triggered(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->rx_finish_isr_cnt);
		SPI_LOGV("rx fifo finish int triggered\r\n");
		if (spi[id].rx_size && spi[id].rx_buf) {
			while (spi_hal_read_byte(hal, &rd_data) == OPRT_OK) {
				if (rd_offset < spi[id].rx_size) {
					spi[id].rx_buf[rd_offset++] = rd_data;
				}
			}
			spi[id].rx_offset = rd_offset;
		}
		spi_clr_rx(id);
		if (spi_rx_finish_isr[id].callback){
			spi_rx_finish_isr[id].callback(id,spi_rx_finish_isr[id].param);
		}
		if (spi[id].is_rx_blocked) {
			rtos_set_semaphore(&spi[id].rx_sema);
			spi[id].is_rx_blocked = false;
		}
	}

	if (spi_hal_is_rx_overflow_int_triggered(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->rx_overflow_isr_cnt);
		SPI_LOGW("rx overflow int triggered\r\n");
	}

	if (spi_hal_is_tx_fifo_int_triggered_with_status(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->tx_fifo_isr_cnt);
		SPI_LOGV("tx fifo int triggered\r\n");
#if !CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
		if ((!spi[id].is_sw_tx_finished) &&
			spi[id].tx_size &&
			spi[id].tx_buf) {
			spi_id_write_bytes_common(id);
			spi[id].is_sw_tx_finished = true;
		}
		for (int i = 0; (i < spi[id].rx_size) && spi[id].rx_buf; i++) {
			/* bk7251 need spi_hal_write_byte when read byte,
			 * bk7251 master read data will not work without this operation */
			if (spi_hal_is_master(hal)) {
				spi_hal_write_byte(hal, 0xff);
			}
			if (spi_hal_read_byte(hal, &rd_data) == OPRT_OK) {
				SPI_LOGV("tx fifo int read byte\r\n");
				if (rd_offset < spi[id].rx_size) {
					spi[id].rx_buf[rd_offset++] = rd_data;
				}
			} else {
				break;
			}
		}
		spi[id].rx_offset = rd_offset;
		if (spi[id].is_sw_tx_finished) {
			spi_hal_disable_tx_fifo_int(hal);
		}
#endif
	}

	if (spi_hal_is_tx_finish_int_triggered(hal, int_status) &&
		spi[id].is_sw_tx_finished) {
		SPI_STATIS_INC(spi_statis->tx_finish_isr_cnt);
		SPI_LOGV("tx finish int triggered\r\n");
		if (spi_hal_is_master(hal)) {
			if (spi[id].is_tx_blocked) {
				rtos_set_semaphore(&spi[id].tx_sema);
				spi[id].is_tx_blocked = false;
			}
			if (spi_tx_finish_isr[id].callback) {
				spi_tx_finish_isr[id].callback(id, spi_tx_finish_isr[id].param);
			}
			spi_clr_tx(id);
		} else {
			SPI_LOGW("tx finish int triggered, but current mode is spi_slave\r\n");
		}
	}

	if (spi_hal_is_tx_underflow_int_triggered(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->tx_underflow_isr_cnt);
		SPI_LOGW("tx underflow int triggered\r\n");
	}

	if (spi_hal_is_slave_release_int_triggered(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->slave_release_isr_cnt);
		SPI_LOGV("slave cs up int triggered\r\n");
		if (spi_hal_is_slave(hal)) {
			if (spi[id].is_tx_blocked) {
				rtos_set_semaphore(&spi[id].tx_sema);
				spi[id].is_tx_blocked = false;
			}
			if (spi_tx_finish_isr[id].callback) {
				spi_tx_finish_isr[id].callback(id, spi_tx_finish_isr[id].param);
			}
		}
	}
}

static void tkl_spi_isr(void)
{
	SPI_STATIS_DEC();
	SPI_STATIS_GET(spi_statis, SPI_ID_0);
	SPI_STATIS_INC(spi_statis->spi_isr_cnt);
	tkl_spi_isr_common(SPI_ID_0);
}

#if (SOC_SPI_MAX_NUM > 1)

static void tkl_spi2_isr(void)
{
	SPI_STATIS_DEC();
	SPI_STATIS_GET(spi_statis, SPI_ID_1);
	SPI_STATIS_INC(spi_statis->spi_isr_cnt);
	tkl_spi_isr_common(SPI_ID_1);
}

#endif

#if (SOC_SPI_MAX_NUM > 2)

static void tkl_spi3_isr(void)
{
	SPI_STATIS_DEC();
	SPI_STATIS_GET(spi_statis, SPI_ID_2);
	SPI_STATIS_INC(spi_statis->spi_isr_cnt);
	tkl_spi_isr_common(SPI_ID_2);
}

#endif

OPERATE_RET tuya_spi_dma_read_bytes_async(spi_id_t id, void *data, uint32_t size)
{
	SPI_RETURN_INVALID_ID(id);
	SPI_RETURN_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);

	int32_t left_len = size;
	uint32_t rx_len = 0;
	uint32_t buf_offset = 0;

	rx_len = left_len;
	uint32_t int_level = spi_enter_critical();
	//set spi trans_len as 0, to increase max trans_len from 4096(spi max length) to 65536(dma max length).
	spi_hal_set_rx_trans_len(&spi[id].hal, 0);
	spi_hal_clear_rx_fifo(&spi[id].hal);
	spi_hal_disable_rx_fifo_int(&spi[id].hal);
	spi_hal_disable_rx_overflow_int(&spi[id].hal);
	spi_exit_critical(int_level);

	tkl_dma_read(spi[id].spi_rx_dma_chan, (uint32_t)data, size);

	spi_hal_enable_rx(&spi[id].hal);
	return OPRT_OK;
}
OPERATE_RET tuya_spi_dma_write_bytes_async(spi_id_t id, const void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);
	SPI_RETURN_INVALID_ID(id);
	SPI_RETURN_ID_NOT_INIT(id);

	int32_t left_len = size;
	uint32_t tx_len = 0;
	uint32_t buf_offset = 0;

	uint32_t int_level = spi_enter_critical();
	spi_hal_clear_tx_fifo(&spi[id].hal);
	//set spi trans_len as 0, to increase max trans_len from 4096(spi max length) to 65536(dma max length).
	spi_hal_disable_tx_fifo_int(&spi[id].hal);
	spi_hal_disable_tx_underflow_int(&spi[id].hal);
	spi_hal_set_tx_trans_len(&spi[id].hal, 0);
	spi_exit_critical(int_level);

	/* Redmine #8131: clean a Cacheable PSRAM stack source before the DMA reads it.
	 * TX only needs a write-back, so this is safe even for the async path. */
	spi_cacheable_stack_clean(data, size);
	tkl_dma_write(spi[id].spi_tx_dma_chan, (uint32_t)data, size);

	spi_hal_enable_tx(&spi[id].hal);
	return OPRT_OK;
}


static OPERATE_RET spi_dma_duplex_xfer(spi_id_t id, const void *tx_data, uint32_t tx_size, void *rx_data, uint32_t rx_size)
{
	SPI_RETURN_NOT_INIT();
	SPI_RETURN_ID_NOT_INIT(id);
	SPI_RETURN_INVALID_ID(id);

	uint32_t tx_len = 0;
	uint32_t rx_len = 0;
	if (tx_size != rx_size) {
		SPI_LOGW("tx and rx size must equal.\r\n");
		return BK_ERR_SPI_DUPLEX_SIZE_NOT_EQUAL;
	}

	uint32_t len = rx_size > 0 ? rx_size : tx_size;
	uint32_t offset = 0;
	uint32_t int_level = 0;

	/* Redmine #8131: clean a Cacheable PSRAM stack TX source and bounce a
	 * Cacheable PSRAM stack RX destination (blocking path, safe to copy back
	 * after completion). */
	void *rx_xfer = rx_data;
	void *rx_bounce = NULL;
	if (rx_data && spi_addr_in_cacheable_stack(rx_data) && (rx_size != 0)) {
		rx_bounce = os_malloc(rx_size);
		if (rx_bounce == NULL) {
			return OPRT_MALLOC_FAILED;
		}
		rx_xfer = rx_bounce;
	}

	int_level = spi_enter_critical();
	if(rx_data) {
		spi_hal_clear_rx_fifo(&spi[id].hal);
		spi_hal_set_rx_trans_len(&spi[id].hal, 0);
		spi_hal_disable_rx_fifo_int(&spi[id].hal);
		spi_hal_disable_rx_overflow_int(&spi[id].hal);
		tkl_dma_read(spi[id].spi_rx_dma_chan, (uint32_t)rx_xfer, rx_size);
	}
	if(tx_data) {
		spi_hal_clear_tx_fifo(&spi[id].hal);
		spi_hal_set_tx_trans_len(&spi[id].hal, 0);
		spi_hal_disable_tx_fifo_int(&spi[id].hal);
		spi_hal_disable_tx_underflow_int(&spi[id].hal);
		spi_cacheable_stack_clean(tx_data, tx_size);
		tkl_dma_write(spi[id].spi_tx_dma_chan, (uint32_t)tx_data, tx_size);
	}
	if(rx_data) {
		spi_hal_enable_rx(&spi[id].hal);
	}
	if(tx_data) {
		spi_hal_enable_tx(&spi[id].hal);
	}
	spi_exit_critical(int_level);
	if(rx_data) {
		spi[id].is_rx_blocked = true;
		rtos_get_semaphore(&spi[id].rx_sema, BEKEN_NEVER_TIMEOUT);
	}
	int_level = spi_enter_critical();
	extern uint32_t dma_wait_to_idle(dma_id_t id);
	if(tx_data) {
		spi_hal_disable_tx(&spi[id].hal);
		spi_hal_disable_tx_fifo_int(&spi[id].hal);
		dma_wait_to_idle(spi[id].spi_tx_dma_chan);
	}
	if(rx_data) {
		spi_hal_disable_rx(&spi[id].hal);
		spi_hal_disable_rx_fifo_int(&spi[id].hal);
		dma_wait_to_idle(spi[id].spi_rx_dma_chan);
	}
	spi_exit_critical(int_level);

	if (rx_bounce != NULL) {
		memcpy(rx_data, rx_bounce, rx_size);
		os_free(rx_bounce);
	}

	return OPRT_OK;
}

// rx isr callback
static void spi_tx_callback_dispatch(spi_id_t id, void *param)
{
    if (spi_irq[id].cb) {
        spi_irq[id].cb((TUYA_SPI_NUM_E)id, TUYA_SPI_EVENT_TX_COMPLETE);
    }
}

// tx isr callback
static void spi_rx_callback_dispatch(spi_id_t id, void *param)
{
    if (spi_irq[id].cb) {
        spi_irq[id].cb((TUYA_SPI_NUM_E)id, TUYA_SPI_EVENT_RX_COMPLETE);
    }
}

OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, const TUYA_SPI_BASE_CFG_T *cfg)
{
    if (port > TUYA_SPI_NUM_3) {
        return OPRT_INVALID_PARM;
    }

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (port >= TUYA_SPI_NUM_2) {
        if((cfg->role == TUYA_SPI_ROLE_SLAVE) || (cfg->role == TUYA_SPI_ROLE_SLAVE_SIMPLEX)) {
            return OPRT_NOT_SUPPORTED;
        }
        TUYA_QSPI_BASE_CFG_T qspi_cfg;

        qspi_cfg.freq_hz = cfg->freq_hz;
        qspi_cfg.use_dma = true;
        qspi_cfg.mode = cfg->mode;
        qspi_cfg.role = TUYA_QSPI_ROLE_MASTER;
        qspi_cfg.type = TUYA_QSPI_TYPE_FLASH;
        qspi_cfg.dma_data_lines = TUYA_QSPI_1WIRE;

        return tkl_qspi_init((port - TUYA_SPI_NUM_2), &qspi_cfg);
    }

    if(spi_driver_init() != OPRT_OK)
        return OPRT_COM_ERROR;

    spi_config[port].wire_mode = SPI_4WIRE_MODE;
    spi_config[port].baud_rate = cfg->freq_hz;

#if (CONFIG_SPI_DMA)
    if (cfg->spi_dma_flags) {
        spi_config[port].dma_mode = SPI_DMA_MODE_ENABLE;
        spi_config[port].spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
        spi_config[port].spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
    }
#endif
    switch (cfg->role) {
        case TUYA_SPI_ROLE_SLAVE:
        case TUYA_SPI_ROLE_SLAVE_SIMPLEX:
            spi_config[port].role = SPI_ROLE_SLAVE;
            break;
        case TUYA_SPI_ROLE_MASTER:
        case TUYA_SPI_ROLE_MASTER_SIMPLEX:
        default:
            spi_config[port].role = SPI_ROLE_MASTER;
            break;
    }

    if (cfg->databits == TUYA_SPI_DATA_BIT16) {
        spi_config[port].bit_width = SPI_BIT_WIDTH_16BITS;
    } else {
        spi_config[port].bit_width = SPI_BIT_WIDTH_8BITS;
    }

    switch (cfg->mode) {
        case TUYA_SPI_MODE0:
            spi_config[port].polarity = SPI_POLARITY_LOW;
            spi_config[port].phase = SPI_PHASE_1ST_EDGE;
            break;
        case TUYA_SPI_MODE1:
            spi_config[port].polarity = SPI_POLARITY_LOW;
            spi_config[port].phase = SPI_PHASE_2ND_EDGE;
            break;
        case TUYA_SPI_MODE2:
            spi_config[port].polarity = SPI_POLARITY_HIGH;
            spi_config[port].phase = SPI_PHASE_1ST_EDGE;
            break;
        case TUYA_SPI_MODE3:
        default:
            spi_config[port].polarity = SPI_POLARITY_HIGH;
            spi_config[port].phase = SPI_PHASE_2ND_EDGE;
            break;
    }

    if (cfg->bitorder == TUYA_SPI_ORDER_LSB2MSB) {
        spi_config[port].bit_order = SPI_LSB_FIRST;
    } else {
        spi_config[port].bit_order = SPI_MSB_FIRST;
    }

    BK_RETURN_ON_ERR(spi_init((spi_id_t)port, &spi_config[port]));
    return OPRT_OK;
}

OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port)
{
    if (port > TUYA_SPI_NUM_3) {
        return OPRT_INVALID_PARM;
    }

    if (port >= TUYA_SPI_NUM_2) {
        return tkl_qspi_deinit(port - TUYA_SPI_NUM_2);
    }
    BK_RETURN_ON_ERR(spi_deinit((spi_id_t)port));
    spi_driver_deinit();
    memset(&spi_config[port], 0, sizeof(spi_config[port]));
    return OPRT_OK;
}

OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, void *data, uint32_t size)
{
    OPERATE_RET ret = OPRT_OK;

    if (data == NULL || port > TUYA_SPI_NUM_3) {
        return OPRT_INVALID_PARM;
    }

    if (port >= TUYA_SPI_NUM_2) {
        return tkl_qspi_send(port - TUYA_SPI_NUM_2, data, size);
    }
#if (CONFIG_SPI_DMA)
    if (SPI_DMA_MODE_ENABLE == spi_config[port].dma_mode) {
        // if (size >= SPI_DMA_MAX_LEN) {
        //     return OPRT_INVALID_PARM;
        // }

        if (spi_irq[port].irq_enable) {
            ret = tuya_spi_dma_write_bytes_async((spi_id_t)port, data, size);
        } else {
            ret = spi_dma_write_bytes((spi_id_t)port, data, size);
        }

        if (ret != OPRT_OK)
            return OPRT_COM_ERROR;
    } else
#endif
    {
        if (spi_irq[port].irq_enable) {
            ret = spi_write_bytes_async((spi_id_t)port, data, size);
        } else {
            ret = spi_write_bytes((spi_id_t)port, data, size);
        }

        if (ret != OPRT_OK)
            return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, void *data, uint32_t size)
{
    OPERATE_RET ret = OPRT_OK;
    if (data == NULL || port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }
    if (port >= TUYA_SPI_NUM_2) {
        if (size <= 256) {
            TUYA_QSPI_CMD_T sd_command;
            sd_command.op = TUYA_QSPI_READ;
            sd_command.addr_size = 0;
            sd_command.cmd_size = 0;
            sd_command.data_size = size;
            sd_command.data = (uint8_t *)data;
            sd_command.cmd_lines = TUYA_QSPI_1WIRE;
            sd_command.addr_lines = TUYA_QSPI_1WIRE;
            sd_command.data_lines = TUYA_QSPI_1WIRE;
            return tkl_qspi_comand(port - TUYA_SPI_NUM_2,  &sd_command);
        }else { // > 256 use dma
            return OPRT_INVALID_PARM;
        }
        // return tkl_qspi_recv(TUYA_QSPI_NUM_0, data, size);
    }

#if (CONFIG_SPI_DMA)
    if (SPI_DMA_MODE_ENABLE == spi_config[port].dma_mode) {

        if (spi_irq[port].irq_enable) {
            ret = tuya_spi_dma_read_bytes_async((spi_id_t)port, data, size);
        } else {
            ret = spi_dma_read_bytes((spi_id_t)port, data, size);
        }

        if (ret != OPRT_OK)
            return OPRT_COM_ERROR;
    } else
#endif
    {
        if (spi_irq[port].irq_enable) {
            ret = spi_read_bytes_async((spi_id_t)port, data, size);
        } else {
            ret = spi_read_bytes((spi_id_t)port, data, size);
        }

        if (ret != OPRT_OK)
            return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, void* send_buf, void* receive_buf, uint32_t length)
{
    if (port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

#if (CONFIG_SPI_DMA)
    if (SPI_DMA_MODE_ENABLE == spi_config[port].dma_mode) {
        // if (length >= SPI_DMA_MAX_LEN) {
        //     return OPRT_INVALID_PARM;
        // }

        spi_dma_duplex_init(port);
        OPERATE_RET ret = spi_dma_duplex_xfer(port, send_buf, length, receive_buf, length);
        spi_dma_duplex_deinit(port);
        if (ret != OPRT_OK) {
            return OPRT_COM_ERROR;
        }
    } else {
#endif

        return OPRT_NOT_SUPPORTED;
#if (CONFIG_SPI_DMA)
    }
#endif
    return OPRT_OK;
}

OPERATE_RET tkl_spi_transfer_with_length(TUYA_SPI_NUM_E port, void* send_buf, uint32_t send_len, void* receive_buf, uint32_t receive_len)
{
    if (port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

#if (CONFIG_SPI_DMA)
    if (SPI_DMA_MODE_ENABLE == spi_config[port].dma_mode) {
        // if (send_len >= SPI_DMA_MAX_LEN || receive_len >= SPI_DMA_MAX_LEN) {
        //     return OPRT_INVALID_PARM;
        // }

        spi_dma_duplex_init(port);
        OPERATE_RET ret = spi_dma_duplex_xfer(port, send_buf, send_len, receive_buf, receive_len);
        spi_dma_duplex_deinit(port);
        if (ret != OPRT_OK) {
            return OPRT_COM_ERROR;
        }
    } else {
#endif
        return OPRT_NOT_SUPPORTED;
#if (CONFIG_SPI_DMA)
    }
#endif
    return OPRT_OK;
}

OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port)
{
    if (port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

    if (spi_deinit((spi_id_t)port) != OPRT_OK)
        return OPRT_COM_ERROR;
    return OPRT_OK;
}

/**
 * @brief spi irq init
 * NOTE: call this API will not enable interrupt
 *
 * @param[in] port: spi port, id index starts at 0
 * @param[in] cb:  spi irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
static void qspi_tx_done_cb(TUYA_QSPI_NUM_E port, TUYA_QSPI_IRQ_EVT_E event)
{
    if (event == TUYA_QSPI_EVENT_TX) {
        if (spi_irq[port + TUYA_SPI_NUM_2].cb) {
            spi_irq[port + TUYA_SPI_NUM_2].cb((TUYA_SPI_NUM_E)TUYA_SPI_NUM_2 + port, TUYA_SPI_EVENT_TX_COMPLETE);
        }
    }
}

OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_CB cb)
{
    if (port > TUYA_SPI_NUM_3) {
        return OPRT_INVALID_PARM;
    }

    spi_irq[port].cb = cb;
    spi_irq[port].irq_enable = 0;

    if (port >= TUYA_SPI_NUM_2) {
        return tkl_qspi_irq_init(port - TUYA_SPI_NUM_2, qspi_tx_done_cb);
    }
    return OPRT_OK;
}

/**
 * @brief spi irq enable
 *
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port)
{
    if (port > TUYA_SPI_NUM_3) {
        return OPRT_INVALID_PARM;
    }

    if (port >= TUYA_SPI_NUM_2) {
        return tkl_qspi_irq_enable(port - TUYA_SPI_NUM_2);
    }

    spi_register_tx_finish_isr((spi_id_t)port, spi_tx_callback_dispatch, NULL);
    spi_register_rx_finish_isr((spi_id_t)port, spi_rx_callback_dispatch, NULL);

    spi_irq[port].irq_enable = 1;

    return OPRT_OK;
}

/**
 * @brief spi irq disable
 *
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port)
{
    if (port > TUYA_SPI_NUM_3) {
        return OPRT_INVALID_PARM;
    }

    if (port >= TUYA_SPI_NUM_2) {
        return tkl_qspi_irq_disable(port - TUYA_SPI_NUM_2);
    }
    spi_register_tx_finish_isr((spi_id_t)port, NULL, NULL);
    spi_register_rx_finish_isr((spi_id_t)port, NULL, NULL);

    spi_irq[port].irq_enable = 0;

    return OPRT_OK;
}

/**
 * @brief spi get max supported dma data length.
 *
 * @param[in] NULL
 *
 * @return >=0,number of supported dma data length. <0,err.
 * during  tkl_spi_send, tkl_spi_recv and tkl_spi_transfer operation.
 */
uint32_t  tkl_spi_get_max_dma_data_length(void)
{
    return SPI_DMA_MAX_LEN - 1;
}
