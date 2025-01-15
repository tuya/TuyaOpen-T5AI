/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "soc/bk7286/reg_base.h"
#include "partitions.h"
#include "sdkconfig.h"

#if (CONFIG_SOC_BK7286_SMP)
#include "smp_id.h"
#endif

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

__MSP_STACK_SIZE = (4 << 10);
__MIN_HEAP_SIZE  = (90 << 10);
__MPU_PROTECT_SIZE = 0x0;

__SWAP_SIZE = (1024);

#if CONFIG_SPE
__CPU0_APP_RAM_BASE = SOC_SRAM0_DATA_BASE;
__TOTAL_RAM_SIZE = CONFIG_CPU0_SPE_RAM_SIZE;
#else
__CPU0_APP_RAM_BASE = SOC_SRAM1_DATA_BASE;
__TOTAL_RAM_SIZE = CONFIG_CPU0_NSPE_RAM_SIZE;
#endif

__CPU0_APP_VIRTUAL_CODE_START = CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_START;
__CPU0_APP_VIRTUAL_CODE_SIZE = CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_SIZE;

MEMORY
{
  FLASH (rx)            : ORIGIN = (SOC_FLASH_DATA_BASE + __CPU0_APP_VIRTUAL_CODE_START), LENGTH = __CPU0_APP_VIRTUAL_CODE_SIZE
  RAM (rwx)             : ORIGIN = __CPU0_APP_RAM_BASE, LENGTH = __TOTAL_RAM_SIZE - __SWAP_SIZE
  SWAP (rwx)            : ORIGIN = __CPU0_APP_RAM_BASE + __TOTAL_RAM_SIZE - __SWAP_SIZE, LENGTH = __SWAP_SIZE

#if (CONFIG_SOC_BK7286_SMP)
  ITCM0 (rwx)           : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM0 (rwx)           : ORIGIN = SOC_DTCM_DATA_BASE + CPU_ID_SPACE_SIZE, LENGTH = CONFIG_DTCM_SIZE - CPU_ID_SPACE_SIZE

  ITCM1 (rwx)           : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM1 (rwx)           : ORIGIN = SOC_DTCM_DATA_BASE + CPU_ID_SPACE_SIZE, LENGTH = CONFIG_DTCM_SIZE - CPU_ID_SPACE_SIZE

  ITCM2 (rwx)           : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM2 (rwx)           : ORIGIN = SOC_DTCM_DATA_BASE + CPU_ID_SPACE_SIZE, LENGTH = CONFIG_DTCM_SIZE - CPU_ID_SPACE_SIZE
#else
  ITCM (rwx)            : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM (rwx)            : ORIGIN = SOC_DTCM_DATA_BASE + 0, LENGTH = CONFIG_DTCM_SIZE
#endif

  PSRAM (rwx)           : ORIGIN = SOC_PSRAM_DATA_BASE, LENGTH = 0x4000000
}

#if (CONFIG_SOC_BK7286_SMP)
ENTRY(Reset_Handler_Cpu0)
#else
ENTRY(Reset_Handler)
#endif

SECTIONS
{
    ASSERT((. == ALIGN(512)), "vector table address align fault.")
    .vectors :
    {
#if (CONFIG_SOC_BK7286_SMP)
        __vector_core0_table = .;
        KEEP(*(.vectors_core0))
        . = ALIGN(512);

        __vector_core1_table = .;
        KEEP(*(.vectors_core1))
        . = ALIGN(512);

        __vector_core2_table = .;
        KEEP(*(.vectors_core2))
        . = ALIGN(512);
#else
        __vector_table = .;
        KEEP(*(.vectors))
        . = ALIGN(512);
#endif
    } > FLASH


    .gnu.sgstubs ALIGN(32) : ALIGN(32)
    {
        *(.gnu.sgstubs*)
        . = ALIGN(32);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        _stext = .;

        . = ALIGN(4);
        __devconfig_start = .;
        *(".devconfig.*")
        KEEP(*(SORT_BY_NAME(".devconfig*")))
        __devconfig_end = .;

        . = ALIGN(4);
        __apps_start = .;
        KEEP (*(.apps_data))
        __apps_end = .;
        _etext = .;
        . = ALIGN(4);
    } > FLASH

    .a_device_null :
    {
      KEEP(*(.a_deviceobj_null))
    } > FLASH

    .a_devices :
    {
      __device_start = .;
      KEEP(*(.a_deviceobj_*))
      __device_end = .;
    } > FLASH

    .a_init_entries :
    {
      __a_init_start = .;
      KEEP(*(.a_init_entry_*))
      __a_init_end = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        LONG (__etext)
        LONG (__iram_start__)
        LONG ((__data_end__ - __iram_start__) / 4)

        LONG (__video_cache_text)
        LONG (__video_cache_data_start__)
        LONG ((__video_cache_data_end__ - __video_cache_data_start__) / 4)

#if ((!defined(CONFIG_SOC_BK7286_SMP)) || (0 == CONFIG_SOC_BK7286_SMP))
        LONG (__itcm_text)
        LONG (__itcm_start__)
        LONG ((__itcm_end__ - __itcm_start__) / 4)

        LONG (__dtcm_content)
        LONG (__dtcm_start__)
        LONG ((__dtcm_end__ - __dtcm_start__) / 4)
#endif
        __copy_table_end__ = .;
    } > FLASH

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (_bss_start)
        LONG ((_bss_end - _bss_start) / 4)
        /* Add each additional bss section here */
        LONG (__video_cache_bss_start__)
        LONG ((__video_cache_bss_end__ - __video_cache_bss_start__) / 4)
        __zero_table_end__ = .;
    } > FLASH

#if (CONFIG_SOC_BK7286_SMP)
    .tcm.copy.table :
    {
        . = ALIGN(4);
        __tcm_copy_table_start__ = .;
        LONG (tcm_cpu0_core_id)
        LONG (__itcm_cpu0_text)
        LONG (__itcm_cpu0_start__)
        LONG ((__itcm_cpu0_end__ - __itcm_cpu0_start__) / 4)

        LONG (tcm_cpu0_core_id)
        LONG (__dtcm_cpu0_content)
        LONG (__dtcm_cpu0_start__)
        LONG ((__dtcm_cpu0_end__ - __dtcm_cpu0_start__) / 4)

        LONG (tcm_cpu1_core_id)
        LONG (__itcm_cpu1_text)
        LONG (__itcm_cpu1_start__)
        LONG ((__itcm_cpu1_end__ - __itcm_cpu1_start__) / 4)

        LONG (tcm_cpu1_core_id)
        LONG (__dtcm_cpu1_content)
        LONG (__dtcm_cpu1_start__)
        LONG ((__dtcm_cpu1_end__ - __dtcm_cpu1_start__) / 4)

        LONG (tcm_cpu2_core_id)
        LONG (__itcm_cpu2_text)
        LONG (__itcm_cpu2_start__)
        LONG ((__itcm_cpu2_end__ - __itcm_cpu2_start__) / 4)

        LONG (tcm_cpu2_core_id)
        LONG (__dtcm_cpu2_content)
        LONG (__dtcm_cpu2_start__)
        LONG ((__dtcm_cpu2_end__ - __dtcm_cpu2_start__) / 4)
        __tcm_copy_table_end__ = .;
    } > FLASH

	tcm_cpu0_core_id = CPU0_CORE_ID;
    .itcm_cpu0 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu0_text = LOADADDR(.itcm_cpu0));
        __itcm_cpu0_start__ = .;
        *(.itcm_cpu0)
        . = ALIGN(4);

        __itcm_cpu0_end__ = .;
    } > ITCM0 AT > FLASH

    .dtcm_cpu0 :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_cpu0_content = LOADADDR(.dtcm_cpu0));
        __dtcm_cpu0_start__ = .;
        *(.dtcm_cpu0)
        __dtcm_cpu0_end__ = .;
    } > DTCM0 AT > FLASH

	tcm_cpu1_core_id = CPU1_CORE_ID;
    .itcm_cpu1 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu1_text = LOADADDR(.itcm_cpu1));
        __itcm_cpu1_start__ = .;
        *(.itcm_cpu1)
        . = ALIGN(4);

        __itcm_cpu1_end__ = .;
    } > ITCM1 AT > FLASH

    .dtcm_cpu1 :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_cpu1_content = LOADADDR(.dtcm_cpu1));
        __dtcm_cpu1_start__ = .;
        *(.dtcm_cpu1)
        __dtcm_cpu1_end__ = .;
    } > DTCM1 AT > FLASH

	tcm_cpu2_core_id = CPU2_CORE_ID;
    .itcm_cpu2 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu2_text = LOADADDR(.itcm_cpu2));
        __itcm_cpu2_start__ = .;
        *(.itcm_cpu2)
        . = ALIGN(4);

        __itcm_cpu2_end__ = .;
    } > ITCM2 AT > FLASH

    .dtcm_cpu2 :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_cpu2_content = LOADADDR(.dtcm_cpu2));
        __dtcm_cpu2_start__ = .;
        *(.dtcm_cpu2)
        __dtcm_cpu2_end__ = .;
    } > DTCM2 AT > FLASH
#else
    .itcm :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_text = LOADADDR(.itcm));
        __itcm_start__ = .;
        KEEP(*(.null_trap_handler))

        . = ALIGN(512);
        __vector_itcm_table = .;
        KEEP(*(.vectors_iram))
        . = ALIGN(512);

        *(.itcm)
        *(.itcm_section*)
        *(.itcm_sec_code*)
        . = ALIGN(4);

        __itcm_end__ = .;
    } > ITCM AT > FLASH

    .dtcm :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_content = LOADADDR(.dtcm));
        __dtcm_start__ = .;
        *(.dtcm)
        *(.dtcm_section*)
        *(.dtcm_sec_data*)

        __dtcm_end__ = .;
    } > DTCM AT > FLASH
#endif

    .data :
    {
        . = ALIGN(512);
        PROVIDE(__etext = LOADADDR(.data));
        PROVIDE(__iram_flash_begin = LOADADDR(.data));
        __iram_start__ = .;

#ifdef CONFIG_GCOV
        /* added in template for gcov: */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
#endif

        *(.iram)
        *(.interrupt)
        __iram_end__ = .;

        . = ALIGN(4);
        __data_start__ = .;
        *(.data)
        *(".data.*")
        *(.sdata)

        *(.video_spec_data*)
        *(.gnu.linkonce.d*)
        __data_end__ = .;
    } > RAM AT > FLASH

    _data_flash_begin = LOADADDR(.data);
    _data_ram_begin = ADDR(.data);
    _data_ram_end = .;

    s_rom_end = LOADADDR(.data) + SIZEOF(.data);

    .uninitialized (NOLOAD):
    {
        . = ALIGN(32);
        __uninitialized_start = .;
        *(.uninitialized)
        *(".uninitialized.*")
        KEEP(*(.keep.uninitialized))
        . = ALIGN(32);
        __uninitialized_end = .;
    } > RAM

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss*)
        *(COMMON)

#if CONFIG_CACHE_ENABLE
        . = ALIGN(4);
        _nocache_start = .;
        *(.sram_nocache*)
        _nocache_end = .;
#endif

        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    bss_size = _bss_end - _bss_start;

#if (CONFIG_SOC_BK7286_SMP)
	_msp_total_size = __MSP_STACK_SIZE * CONFIG_CPU_CNT;
#else
	_msp_total_size = __MSP_STACK_SIZE;
#endif

    .heap (COPY) :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = . + (ORIGIN(RAM) + LENGTH(RAM) - _msp_total_size - _heap_start - 8); /* 16 bytes for boundary protection */
        . = ALIGN(8);
        _heap_end = .;
    } > RAM

/*multiple cores require independent stack spaces*/
#if (CONFIG_SOC_BK7286_SMP)
	cpu2_stack_dummy_size = 8; /*[MPW V5]TODO:actually, it is __MSP_STACK_SIZE*/
	ASSERT((CONFIG_CPU_CNT == 2), "cpu count exceptional!!!")
    .stack_cpu2 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE*2 - cpu2_stack_dummy_size) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu2 = .;
        __StackLimitCpu2 = .;
        . = . + cpu2_stack_dummy_size;
        . = ALIGN(8);
        __StackTopCpu2 = .;
        _estack_cpu2 = .;
    } > RAM

    .stack_cpu1 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE*2) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu1 = .;
        __StackLimitCpu1 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCpu1 = .;
        _estack_cpu1 = .;
    } > RAM

    .stack_cpu0 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu0 = .;
        __StackLimitCpu0 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCpu0 = .;
        _estack_cpu0 = .;
    } > RAM
#else
    .stack (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack = .;
        __StackLimit = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTop = .;
        _estack = .;
    } > RAM
#endif

    .swap ORIGIN(SWAP):
    {
        . = ALIGN(8);
        _swap_start = .;

        * (.swap_data)
        * (.swap_data*)
        _swap_end = .;
    } > SWAP AT > FLASH

    .video.cache.data :
    {
        . = ALIGN(4);
        PROVIDE(__video_cache_text = LOADADDR(.video.cache.data));
        __video_cache_data_start__ = .;

        *(.video_cache_data)
        *(.video_cache_data*)

        . = ALIGN(4);
        __video_cache_data_end__ = .;
    } > PSRAM AT > FLASH

    .video_cache_bss (NOLOAD):
    {
        . = ALIGN(4);
        __video_cache_bss_start__ = .;
        *(.video_cache_bss)
        *(.video_cache_bss*)
        . = ALIGN(4);
        __video_cache_bss_end__ = .;
    } > PSRAM

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __MSPTop = ORIGIN(RAM) + LENGTH(RAM);
    __MSPLimit = __MSPTop - __MSP_STACK_SIZE;

    ASSERT((s_rom_end < ORIGIN(FLASH) + LENGTH(FLASH)), "ROM overflow!!!")
    ASSERT(((ORIGIN(RAM) + LENGTH(RAM)) > (_heap_start + __MSP_STACK_SIZE + 8)), "Stack overflowed with bss")
    ASSERT(((_heap_end - _heap_start) >= __MIN_HEAP_SIZE), "Heap smaller than minimize size 90K!!!")
}
//eof

