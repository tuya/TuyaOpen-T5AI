/*
 * test_audio.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"

#include "sys_driver.h"
#include <driver/aud_adc_types.h>
#include <driver/aud_adc.h>
#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>
#include <driver/i2s.h>
#include <driver/i2s_types.h>
#include <driver/dma.h>

#define ADC_FRAME_SIZE    (320)
static dma_id_t adc_dma_id = 0;
static dma_id_t dac_dma_id = 0;
static uint8_t *adc_ringbuff_addr = NULL;
static uint8_t *dac_ringbuff_addr = NULL;
static RingBufferContext adc_rb;
static RingBufferContext dac_rb;
static uint16_t *adc_temp = NULL;
static uint8_t test_mic_id = 0;


static void tuya_audio_adc_dma_finish_isr(void)
{
    /* read adc data from adc ringbuffer */
    uint32_t size = ring_buffer_read(&adc_rb, (uint8_t*)adc_temp, ADC_FRAME_SIZE);
    if (size != ADC_FRAME_SIZE) {
        return;
    }

    /* select r channel data */
    for (uint32_t i = 0; i < ADC_FRAME_SIZE/2; i++) {
        adc_temp[i] = adc_temp[2*i+1];
    }

    size = ring_buffer_get_free_size(&dac_rb);
    if (size >= ADC_FRAME_SIZE/2) {
        ring_buffer_write(&dac_rb, (uint8_t *)adc_temp, ADC_FRAME_SIZE/2);
    }
}

void tuya_audio_adc_mic2_to_dac_test(uint32_t state, uint32_t sample_rate)
{
    aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
    adc_config.adc_chl = AUD_ADC_CHL_LR;
    adc_config.clk_src = AUD_CLK_XTAL;
    adc_config.samp_rate = sample_rate;
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    dac_config.clk_src = AUD_CLK_XTAL;
    dac_config.samp_rate = sample_rate;
    dma_config_t dma_config = {0};
    dma_config_t dac_dma_config = {0};
    uint32_t aud_adc_data_addr;
    uint32_t aud_dac_data_addr;
    bk_err_t ret = BK_OK;

    if (state == 2) {

        bk_aud_adc_init(&adc_config);

        //sys_drv_aud_mic1_gain_set(0);
        sys_drv_aud_mic2_gain_set(0);

        //disable audio interrupt when loop test
        sys_drv_aud_int_en(0);
        bk_aud_adc_disable_int();

        //start adc
        //	bk_aud_adc_start();

        /* init dma driver */
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            return;
        }

        /* allocate free DMA channel */
        adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((adc_dma_id < DMA_ID_0) || (adc_dma_id >= DMA_ID_MAX)) {
            return;
        }

        dma_config.mode = DMA_WORK_MODE_REPEAT;
        dma_config.chan_prio = 1;
        dma_config.src.width = DMA_DATA_WIDTH_32BITS;
        dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
        dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
        dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
        dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.trans_type = DMA_TRANS_DEFAULT;

        /* get audio adc and dac fifo address */
        bk_aud_adc_get_fifo_addr(&aud_adc_data_addr);

        /* init ringbuff */
        adc_ringbuff_addr = (uint8_t *)os_malloc(ADC_FRAME_SIZE*2);
        ring_buffer_init(&adc_rb, adc_ringbuff_addr, ADC_FRAME_SIZE*2, adc_dma_id, RB_DMA_TYPE_WRITE);

        adc_temp = (uint16_t *)os_malloc(ADC_FRAME_SIZE);

        /* audio adc to dtcm by dma */
        dma_config.src.dev = DMA_DEV_AUDIO_RX;
        dma_config.dst.dev = DMA_DEV_DTCM;
        dma_config.dst.start_addr = (uint32_t)adc_ringbuff_addr;
        dma_config.dst.end_addr = (uint32_t)adc_ringbuff_addr + ADC_FRAME_SIZE*2;
        dma_config.src.start_addr = aud_adc_data_addr;
        dma_config.src.end_addr = aud_adc_data_addr + 4;

        /* init dma channel */
        ret = bk_dma_init(adc_dma_id, &dma_config);
        if (ret != BK_OK) {
            return;
        }

        /* set dma transfer length */
        bk_dma_set_transfer_len(adc_dma_id, ADC_FRAME_SIZE);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(adc_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(adc_dma_id, DMA_ATTR_SEC);
#endif

        //register isr
        bk_dma_register_isr(adc_dma_id, NULL, (void *)tuya_audio_adc_dma_finish_isr);
        bk_dma_enable_finish_interrupt(adc_dma_id);

        /* dac config */
        bk_aud_dac_init(&dac_config);

        sys_drv_aud_dacg_set(0);

        //disable audio interrupt when loop test
        //	bk_aud_dac_disable_int();

        //start dac
        //	bk_aud_dac_start();

        /* allocate free DMA channel */
        dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((dac_dma_id < DMA_ID_0) || (dac_dma_id >= DMA_ID_MAX)) {
            return;
        }

        dac_dma_config.mode = DMA_WORK_MODE_REPEAT;
        dac_dma_config.chan_prio = 1;
        dac_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
        dac_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
        dac_dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
        dac_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dac_dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
        dac_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dac_dma_config.trans_type = DMA_TRANS_DEFAULT;

        /* get audio adc and dac fifo address */
        bk_aud_dac_get_fifo_addr(&aud_dac_data_addr);

        /* init ringbuff */
        dac_ringbuff_addr = (uint8_t *)os_malloc(ADC_FRAME_SIZE);
        ring_buffer_init(&dac_rb, dac_ringbuff_addr, ADC_FRAME_SIZE, dac_dma_id, RB_DMA_TYPE_READ);

        /* audio adc to dtcm by dma */
        dac_dma_config.src.dev = DMA_DEV_DTCM;
        dac_dma_config.dst.dev = DMA_DEV_AUDIO;
        dac_dma_config.dst.start_addr = aud_dac_data_addr;
        dac_dma_config.dst.end_addr = aud_dac_data_addr + 2;
        dac_dma_config.src.start_addr = (uint32_t)dac_ringbuff_addr;
        dac_dma_config.src.end_addr = (uint32_t)dac_ringbuff_addr + ADC_FRAME_SIZE;

        /* init dma channel */
        ret = bk_dma_init(dac_dma_id, &dac_dma_config);
        if (ret != BK_OK) {
            return;
        }

        /* set dma transfer length */
        bk_dma_set_transfer_len(dac_dma_id, ADC_FRAME_SIZE/2);

        os_memset(adc_temp, 0, ADC_FRAME_SIZE/2);
        ring_buffer_write(&dac_rb, (uint8_t *)adc_temp, ADC_FRAME_SIZE/2);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(dac_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(dac_dma_id, DMA_ATTR_SEC);
#endif

        /* start dac and adc */
        bk_aud_dac_start();
        bk_aud_dac_start();
        bk_aud_adc_start();

        /* start dma */
        bk_dma_start(dac_dma_id);
        bk_dma_start(adc_dma_id);

        test_mic_id = 2;
    } else if (state == 0) {
        /* check test mic id */
        if (test_mic_id != 2) {
            return;
        }

        for (uint8_t i = 0; i < DMA_ID_MAX; i++) {
            if (bk_dma_user(i) == DMA_DEV_AUDIO) {
                bk_dma_stop(i);
                bk_dma_deinit(i);
                bk_dma_free(DMA_DEV_AUDIO, i);
            }
        }

        bk_dma_driver_deinit();

        if (adc_ringbuff_addr) {
            ring_buffer_clear(&adc_rb);
            os_free(adc_ringbuff_addr);
            adc_ringbuff_addr = NULL;
        }

        if (dac_ringbuff_addr) {
            ring_buffer_clear(&dac_rb);
            os_free(dac_ringbuff_addr);
            dac_ringbuff_addr = NULL;
        }

        if (adc_temp) {
            os_free(adc_temp);
            adc_temp =NULL;
        }

        bk_aud_adc_stop();
        bk_aud_adc_deinit();

        bk_aud_dac_stop();
        bk_aud_dac_deinit();

        test_mic_id = 0;
    }else {
        //not need todo
        return;
    }
}

static void tuya_audio_auto_loop_test(uint32_t state, uint32_t sample_rate, uint32_t dac_mode)
{
    aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();

    if (state == 1) {
        adc_config.adc_chl = AUD_ADC_CHL_LR;
        adc_config.clk_src = AUD_CLK_XTAL;
        dac_config.dac_chl = AUD_DAC_CHL_LR;
        dac_config.clk_src = AUD_CLK_XTAL;
        adc_config.samp_rate = sample_rate;
        dac_config.samp_rate = sample_rate;

        if (dac_mode == 0) {
            dac_config.work_mode = AUD_DAC_WORK_MODE_DIFFEN;
        } else if (dac_mode == 1) {
            dac_config.work_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
        } else {
            dac_config.work_mode = AUD_DAC_WORK_MODE_DIFFEN;
        }

        /* audio process test */
        sys_drv_aud_dacg_set(0);
        sys_drv_aud_mic1_gain_set(0);
        sys_drv_aud_mic2_gain_set(0);

        bk_aud_adc_init(&adc_config);
        bk_aud_dac_init(&dac_config);

        //disable audio interrupt when loop test
        sys_drv_aud_int_en(0);
        bk_aud_adc_disable_int();

        //start adc
        bk_aud_adc_start();
        //start adc
        bk_aud_dac_start();

        //enable adc to dac loop test
        bk_aud_adc_start_loop_test();
    }else {
        //disable adc and dac
        bk_aud_adc_stop();
        bk_aud_dac_stop();

        //stop loop test
        bk_aud_adc_stop_loop_test();
        bk_aud_adc_deinit();
        bk_aud_dac_deinit();
    }
}


// test cli commond:
// audio_test: [test_mode: 0-close, 1-mic1_to_spk, 2-tmic2_to_spk] [sample_rate: 8000, 16000, 44100, 48000] [dac_mode: 0-single, 1-different]
// eg: audio_test 2 16000 1 ------ test mic2 to spk, samplerate 16000, dac mode different
void cli_audio_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("%s %s %s %s\n\r", argv[1],  argv[2],  argv[3],  argv[4]);
    uint32_t state = (uint8_t)os_strtoul(argv[1], NULL, 10);
    uint32_t sample_rate = os_strtoul(argv[2], NULL, 10);
    uint32_t dac_mode = (uint8_t)os_strtoul(argv[3], NULL, 10);

    uint32_t mic1_config = 0;
    bk_printf("state: %d, sample_rate: %d, dac_mode: %d\n\r", state, sample_rate, dac_mode);

    tuya_audio_auto_loop_test(state, sample_rate, dac_mode);
    tuya_audio_adc_mic2_to_dac_test(state, sample_rate);
}



