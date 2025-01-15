#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include "components/system.h"
#include "components/log.h"
#include "driver/flash.h"
#include "modules/ota.h"
#include "utils_httpc.h"
#include "modules/wifi.h"
#include "bk_https.h"
#include "security_ota.h"

#if CONFIG_PSA_MBEDTLS
#include "psa/crypto.h"
#endif


#define TAG "HTTPS_OTA"

#define IMAGE_HEADER_SIZE 5120
#define DEFAULT_OTA_BUF_SIZE IMAGE_HEADER_SIZE

security_ota_cb_t security_ota_cb = NULL;

void security_ota_dispatch_event(security_ota_event_id_t event_id, void* event_data, int event_data_len)
{
	security_ota_event_t event;
	event.event_id = event_id;
	event.data = event_data;
	event.data_len = event_data_len;
	if(security_ota_cb != NULL)
		security_ota_cb(event);
}

bk_http_client_handle_t bk_https_client_flash_init(security_ota_config_t ota_config)
{
	bk_http_client_handle_t client = NULL;

#if CONFIG_SYSTEM_CTRL
	bk_wifi_ota_dtim(1);
#endif
	security_ota_cb = ota_config.ota_event_handler;
	security_ota_init(ota_config.http_config->url);
	client = bk_http_client_init(ota_config.http_config);

	return client;

}

bk_err_t bk_https_client_flash_deinit(bk_http_client_handle_t client)
{
	int err;

#if CONFIG_SYSTEM_CTRL
	bk_wifi_ota_dtim(0);
#endif

	if(client)
		err = bk_http_client_cleanup(client);
	else
		return BK_FAIL;

	return err;

}

bk_err_t https_ota_event_cb(bk_http_client_event_t *evt)
{
    if(!evt) {
        return BK_FAIL;
    }

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
	break;
    case HTTP_EVENT_ON_CONNECTED:
#ifdef CONFIG_HTTP_OTA_WITH_BLE
#if CONFIG_BLUETOOTH
	bk_ble_register_sleep_state_callback(ble_sleep_cb);
#endif
#endif
	break;
    case HTTP_EVENT_HEADER_SENT:
	break;
    case HTTP_EVENT_ON_HEADER:
	break;
    case HTTP_EVENT_ON_DATA:
	security_ota_parse_data((char *)evt->data,evt->data_len);
	break;
    case HTTP_EVENT_ON_FINISH:
	// bk_https_client_flash_deinit(evt->client);
	break;
    case HTTP_EVENT_DISCONNECTED:
	break;

    }
    return BK_OK;
}

int bk_https_ota_download(security_ota_config_t ota_config)
{
	int err = 0;

	bk_http_client_handle_t client = bk_https_client_flash_init(ota_config);
	if (client == NULL) {
		BK_LOGI(TAG, "client is NULL\r\n");
		err = BK_FAIL;
		return err;
	}
	char resume_download[20];
	snprintf(resume_download,20,"bytes=%u-", security_ota_get_restart());
	bk_http_client_set_header(client, "Range", resume_download);

	err = bk_http_client_perform(client);
	if(err == BK_OK){
		BK_LOGI(TAG, "bk_http_client_perform ok\r\n");
		err = security_ota_finish();
	} else {
		bk_https_client_flash_deinit(client);
		BK_LOGI(TAG, "bk_http_client_perform fail, err:%x\r\n", err);
		return err;
	}
	bk_https_client_flash_deinit(client);

	return err;
}

static bool redirection_required(int status_code)
{
	switch (status_code) {
		case HttpStatus_MovedPermanently:
		case HttpStatus_Found:
		case HttpStatus_SeeOther:
		case HttpStatus_TemporaryRedirect:
		case HttpStatus_PermanentRedirect:
			return true;
		default:
			return false;
	}
	return false;
}

static bool process_again(int status_code)
{
	switch (status_code) {
		case HttpStatus_MovedPermanently:
		case HttpStatus_Found:
		case HttpStatus_SeeOther:
		case HttpStatus_TemporaryRedirect:
		case HttpStatus_PermanentRedirect:
		case HttpStatus_Unauthorized:
			return true;
		default:
			return false;
	}
	return false;
}

static bk_err_t _http_handle_response_code(bk_http_client_handle_t http_client, int status_code)
{
	bk_err_t err;
	if (redirection_required(status_code)) {
		err = bk_http_client_set_redirection(http_client);
		if (err != BK_OK) {
			BK_LOGE(TAG, "URL redirection Failed");
			return err;
		}
	} else if (status_code == HttpStatus_Unauthorized) {
		BK_LOGE(TAG, "HttpStatus_Unauthorized\r\n");
		bk_http_client_add_auth(http_client);
	} else if(status_code == HttpStatus_NotFound || status_code == HttpStatus_Forbidden) {
		BK_LOGE(TAG, "File not found(%d)", status_code);
		return BK_FAIL;
	} else if (status_code >= HttpStatus_BadRequest && status_code < HttpStatus_InternalError) {
		BK_LOGE(TAG, "Client error (%d)", status_code);
		return BK_FAIL;
	} else if (status_code >= HttpStatus_InternalError) {
		BK_LOGE(TAG, "Server error (%d)", status_code);
		return BK_FAIL;
	}
	else {
		BK_LOGE(TAG, "status_code: (%d)", status_code);
	}

	char upgrade_data_buf[DEFAULT_OTA_BUF_SIZE];
	// process_again() returns true only in case of redirection.
	if (process_again(status_code)) {
		 BK_LOGE(TAG, "_http_handle_response_code status_code: %d\r\n", status_code);
		while (1) {
			/*
			 *	In case of redirection, bk_http_client_read() is called
			 *	to clear the response buffer of http_client.
			 */
			int data_read = bk_http_client_read(http_client, upgrade_data_buf, DEFAULT_OTA_BUF_SIZE);
			BK_LOGE(TAG, "_http_handle_response_code data_read: !!!!!!!!!!%d\r\n", data_read);
			if (data_read <= 0) {
				return BK_OK;
			}
		}
	}
	return BK_OK;
}

static bk_err_t _http_connect(bk_http_client_handle_t http_client)
{
	bk_err_t err = BK_FAIL;
	int status_code, header_ret;
	do {
		char *post_data = NULL;
		/* Send POST request if body is set.
		 * Note: Sending POST request is not supported if partial_http_download
		 * is enabled
		 */
		int post_len = bk_http_client_get_post_field(http_client, &post_data);
		err = bk_http_client_open(http_client, post_len);
		if (err != BK_OK) {
			BK_LOGE(TAG, "Failed to open HTTP connection\r\n");
			return err;
		}
		if (post_len) {
			BK_LOGI(TAG, "post_len:%d\r\n", post_len);
			int write_len = 0;
			while (post_len > 0) {
				write_len = bk_http_client_write(http_client, post_data, post_len);
				if (write_len < 0) {
					BK_LOGE(TAG, "Write failed\r\n");
					return BK_FAIL;
				}
				post_len -= write_len;
				post_data += write_len;
			}
		}
		BK_LOGI(TAG, "BEGIN FEATCH HEADER\r\n");
		header_ret = bk_http_client_fetch_headers(http_client);
		if (header_ret < 0) {
			BK_LOGE(TAG, "bk_http_client_fetch_headers fail\r\n");
			return header_ret;
		}
		else {
			BK_LOGD(TAG, "header_ret:%d\r\n", header_ret);
		}
		status_code = bk_http_client_get_status_code(http_client);
		err = _http_handle_response_code(http_client, status_code);
		if (err != BK_OK) {
			return err;
		}
	} while (process_again(status_code));
	BK_LOGD(TAG, "_http_connect over\r\n");
	return err;
}

static void _http_cleanup(bk_http_client_handle_t client)
{
	bk_http_client_close(client);
	bk_http_client_cleanup(client);
}
