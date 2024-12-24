#include "tuya_error_code.h"
#include "sys_driver.h"
#include "aud_driver.h"
#include <components/aud_intf.h>
#include <components/aud_intf_types.h>
#include "aud_intf_private.h"
#include "aud_tras_drv.h"

static aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
static aud_intf_mic_setup_t aud_intf_mic_setup = DEFAULT_AUD_INTF_MIC_SETUP_CONFIG();
static aud_intf_work_mode_t aud_work_mode = AUD_INTF_WORK_MODE_NULL;

extern void bk_printf(const char *fmt, ...);

static int aud_handle_mic_data(uint8_t *data, unsigned int len)
{
	//bk_printf("len: %d\r\n", len);
	return len;
}

/**
 * @brief tuya kernel mic init
 *
 * TODO
 * @param[in] func: ADD Callback function
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_mic_init(void)
{
    int ret;

    tkl_semaphore_create_init(&aud_intf_info.api_info.sem, 0, 1);

	//aud_intf_drv_setup.aud_intf_rx_spk_data = aud_read_sd_write2spk;
	aud_intf_drv_setup.aud_intf_tx_mic_data = aud_handle_mic_data;
    ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
    if (ret != BK_ERR_AUD_INTF_OK) {
        bk_printf("bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
    } else {
        bk_printf("bk_aud_intf_drv_init complete \r\n");
    }

    aud_work_mode = AUD_INTF_WORK_MODE_GENERAL;
    ret = bk_aud_intf_set_mode(aud_work_mode);
    if (ret != BK_ERR_AUD_INTF_OK) {
        bk_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
    } else {
        bk_printf("bk_aud_intf_set_mode complete \r\n");
    }

    ret = bk_aud_intf_mic_init(&aud_intf_mic_setup);
    if (ret != BK_ERR_AUD_INTF_OK) {
        bk_printf("bk_aud_intf_mic_init fail, ret:%d \r\n", ret);
    } else {
        bk_printf("bk_aud_intf_mic_init complete \r\n");
    }

    bk_printf("init mic complete \r\n");
}

int tkl_mic_start(void)
{
    int ret = bk_aud_intf_mic_start();
    bk_printf("start mic record %d\r\n", ret);
    return ret;
}

int tkl_mic_pause(void)
{
    int ret = bk_aud_intf_mic_pause();
    bk_printf("start mic record %d\r\n", ret);
    return ret;
}

int tkl_mic_stop(void)
{
    int ret = bk_aud_intf_mic_stop();
    bk_printf("start mic record %d\r\n", ret);
    return ret;
}

void tkl_mic_deinit(void)
{
    int ret = bk_aud_intf_mic_deinit();
    if (ret != BK_ERR_AUD_INTF_OK) {
        bk_printf("bk_aud_intf_mic_deinit fail, ret:%d \r\n", ret);
    } else {
        bk_printf("bk_aud_intf_mic_deinit complete \r\n");
    }

    ret = bk_aud_intf_drv_deinit();
    if (ret != BK_ERR_AUD_INTF_OK) {
        bk_printf("bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
    } else {
        bk_printf("bk_aud_intf_drv_deinit complete \r\n");
    }

    bk_printf("stop mic record \r\n");
}