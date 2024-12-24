#include "cli.h"
#include "modules/ota.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif
#if CONFIG_SECURITY_OTA
#include "bk_https.h"
#include "_ota.h"
#include "security_ota.h"

#if CONFIG_TFM_FLASH_NSC
#include "tfm_flash_nsc.h"
#include <driver/flash_types.h>
#include "partitions.h"
#endif
#endif

#if CONFIG_OTA_TFTP
extern void tftp_start(void);
extern void string_to_ip(char *s);
static void tftp_ota_get_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	short len = 0, i;
	extern char     BootFile[] ;

	if (argc > 3) {
		os_printf("ota server_ip ota_file\r\n");
		return;
	}

	os_printf("%s\r\n", argv[1]);

	os_strcpy(BootFile, argv[2]);
	os_printf("%s\r\n", BootFile);
	string_to_ip(argv[1]);


	tftp_start();

	return;

}
#endif

#if CONFIG_HTTP_AB_PARTITION
void get_http_ab_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	exec_flag ret_partition = 0;
#if CONFIG_OTA_POSITION_INDEPENDENT_AB
	ret_partition = bk_ota_get_current_partition();
	if(ret_partition == 0x0)
	{
    	os_printf("partition A\r\n");
    }
	else
	{
    	os_printf("partition B\r\n");
    }
#else
	ret_partition = bk_ota_get_current_partition();
	if((ret_partition == 0xFF) ||(ret_partition == EXEX_A_PART))
	{
    	os_printf("partition A\r\n");
    }
	else
	{
    	os_printf("partition B\r\n");
    }
#endif
}
#endif

#if CONFIG_DIRECT_XIP && CONFIG_SECURITY_OTA
void get_http_ab_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern uint32_t flash_get_excute_enable();
	uint32_t id = flash_get_excute_enable();
	if(id == 0){
		os_printf("partition A\r\n");
	} else if (id == 1){
		os_printf("partition B\r\n");
	}
}
#endif


#if CONFIG_OTA_HTTP
void http_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret;
	if (argc != 2)
		goto HTTP_CMD_ERR;
	ret = bk_http_ota_download(argv[1]);

	if (0 != ret)
		os_printf("http_ota download failed.");

	return;

HTTP_CMD_ERR:
	os_printf("Usage:http_ota [url:]\r\n");
}
#endif

#if CONFIG_OTA_HTTPS

#define TAG "HTTPS_OTA"
#define HTTPS_INPUT_SIZE   (5120)
static beken_thread_t ota_thread_handle = NULL;

char *https_url = NULL;
#if CONFIG_SECURITY_OTA
/* this crt for url https://docs.bekencorp.com , support test*/
const char ca_crt_rsa[] = {
"-----BEGIN CERTIFICATE-----\r\n"
"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\r\n"
"clib3BcxKZX9qK4A6FAATghuSQ==\r\n"
"-----END CERTIFICATE-----\r\n"
};

int bk_https_ota_download(security_ota_config_t config);

void ota_event_cb(security_ota_event_t evt)
{
	switch (evt.event_id) {
	case SECURITY_OTA_START:
		break;
	case SECURITY_OTA_ERROR:
		BK_LOGE(TAG,"OTA ERROR CODE %d\r\n",*(int*)(evt.data));
		break;
	case SECURITY_OTA_SUCCESS:
#if CONFIG_OTA_UPDATE_PUBKEY
		if (update_back_pubkey_from_primary() != BK_OK) {
			BK_LOGE(TAG, "Back pubkey fail!\r\n");
			break;
		}
#endif
#if CONFIG_OTA_CONFIRM_UPDATE
		if (bk_ota_confirm_update() != BK_OK) {
			BK_LOGE(TAG,"set confirm fail!\r\n");
			break;
		}
#endif
		BK_LOGI(TAG,"OTA SUCCESS\r\n");
		break;	
	case SECURITY_OTA_RESTART:
		break;
	}
}

bk_err_t https_event_cb(bk_http_client_event_t *evt)
{
	switch (evt->event_id) {
	case HTTP_EVENT_ERROR:
		BK_LOGE(TAG, "HTTPS_EVENT_ERROR\r\n");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		BK_LOGI(TAG, "HTTPS_EVENT_ON_CONNECTED\r\n");
		break;
	case HTTP_EVENT_HEADER_SENT:
		BK_LOGI(TAG, "HTTPS_EVENT_HEADER_SENT\r\n");
		break;
	case HTTP_EVENT_ON_HEADER:
		BK_LOGI(TAG, "HTTPS_EVENT_ON_HEADER\r\n");
		break;
	case HTTP_EVENT_ON_DATA:
		security_ota_parse_data((char *)evt->data,evt->data_len);
		BK_LOGD(TAG, "HTTP_EVENT_ON_DATA, length:%d\r\n", evt->data_len);
		break;
	case HTTP_EVENT_ON_FINISH:
		BK_LOGI(TAG, "HTTPS_EVENT_ON_FINISH\r\n");
		break;
	case HTTP_EVENT_DISCONNECTED:
		BK_LOGI(TAG, "HTTPS_EVENT_DISCONNECTED\r\n");
		break;

	}
	return BK_OK;
}

void bk_https_start_download(beken_thread_arg_t arg) {
	int ret;

	bk_http_input_t config = {
	    .url = https_url,
	    .cert_pem = ca_crt_rsa,
	    .event_handler = https_event_cb,
	    .buffer_size = HTTPS_INPUT_SIZE,
		.user_agent = "TEST HTTP Client/1.0",
	    .timeout_ms = 15000,
		.keep_alive_enable = true,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
		.crt_bundle_attach = esp_crt_bundle_attach,
#endif
	};

	security_ota_config_t ota_config = {
		.http_config = &config,
		.ota_event_handler = ota_event_cb,
	};

	ret = bk_https_ota_download(ota_config);
	if(ret != BK_OK) {
		BK_LOGE(TAG, "%s download fail, ret:%d\r\n", __func__, ret);
	}
	ota_thread_handle = NULL;
	rtos_delete_thread(NULL);
}
#else

int bk_https_ota_download(const char *url);
void bk_https_start_download(beken_thread_arg_t arg) {
	int ret;
	ret = bk_https_ota_download(https_url);
	if(ret != BK_OK) {
		BK_LOGE(TAG, "%s download fail, ret:%d\r\n", __func__, ret);
	}
	ota_thread_handle = NULL;
	rtos_delete_thread(NULL);
}

#endif /*SECURITY_OTA*/

void https_ota_start(void)
{
	UINT32 ret = 0;

	BK_LOGI(TAG, "https_ota_start\r\n");
	if(ota_thread_handle == NULL) {
		ret = rtos_create_thread(&ota_thread_handle, BEKEN_APPLICATION_PRIORITY,
								"https_ota",
								(beken_thread_function_t)bk_https_start_download,
								5120,
								0);
	}
	if (kNoErr != ret)
		BK_LOGE(TAG, "https_ota_start failed\r\n");

}

void https_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

	if (argc != 2)
		goto HTTP_CMD_ERR;

	https_url = argv[1];
	https_ota_start();

	return;

HTTP_CMD_ERR:
	BK_LOGE(TAG, "%s,Usage:http_ota [url:]\r\n",__func__);
}
#endif

#if CONFIG_OTA_CONFIRM_UPDATE
void ota_confirm_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 2) {
		BK_LOGE(TAG, "ota_confirm {confirm_update|cancel_update}\r\n");
		return;
	}

	if (os_strcmp(argv[1], "confirm_update") == 0){
		bk_ota_confirm_update();
		BK_LOGI(TAG, "confirm udpate\r\n");
	} else if (os_strcmp(argv[1], "cancel_update") == 0){
		bk_ota_cancel_update(0);
		BK_LOGI(TAG, "cancel update\r\n");
	}
}
#endif

#if defined(CONFIG_SECURITY_OTA) && !defined(CONFIG_TFM_FWU)
void get_primary_sign_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t* version = (uint8_t*)os_malloc(4*sizeof(uint8_t));
	bk_err_t ret = bk_read_sign_version_from_primary(version);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "read app_version fail\r\n");
		return;
	}
	uint16_t num = (uint16_t)version[2] | ((uint16_t)version[3] << 8);
	BK_LOGI(TAG, "app_version:%d.%d.%d\r\n",version[0],version[1],num);
	os_free(version);
}
#endif

#if CONFIG_SECURITY_OTA && CONFIG_TFM_FLASH_NSC
void ota_brick_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	psa_flash_set_protect_type(FLASH_PROTECT_NONE);
	uint8_t zero_buf[64];
	memset(zero_buf, 0x0, sizeof(zero_buf));
	uint32_t brick_addr;
	uint32_t brick_offset = 8*1024;
	if (os_strcmp(argv[1], "brick_primary") == 0){
		brick_addr = CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + brick_offset;
		psa_flash_write_bytes(brick_addr, zero_buf, sizeof(zero_buf));
	} else if (os_strcmp(argv[1], "brick_ota") == 0){
		brick_addr = CONFIG_OTA_PHY_PARTITION_OFFSET + brick_offset;
		psa_flash_write_bytes(brick_addr, zero_buf, sizeof(zero_buf));
	}
	psa_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
}
#endif

#define OTA_CMD_CNT (sizeof(s_ota_commands) / sizeof(struct cli_command))
static const struct cli_command s_ota_commands[] = {

#if CONFIG_OTA_TFTP
	{"tftpota", "tftpota [ip] [file]", tftp_ota_get_Command},
#endif

#if CONFIG_OTA_HTTP && !CONFIG_SECURITY_OTA
	{"http_ota", "http_ota url", http_ota_Command},
#endif

#if CONFIG_OTA_HTTPS
	{"https_ota", "ip [sta|ap][{ip}{mask}{gate}{dns}]", https_ota_Command},
#endif

#if CONFIG_OTA_CONFIRM_UPDATE && CONFIG_SECURITY_OTA
	{"ota_confirm", " ", ota_confirm_Command},
#endif

#if CONFIG_HTTP_AB_PARTITION || (CONFIG_DIRECT_XIP && CONFIG_SECURITY_OTA)
	{"ab_version", NULL, get_http_ab_version},
#endif

#if defined(CONFIG_SECURITY_OTA) && !defined(CONFIG_TFM_FWU)
	{"app_version", NULL, get_primary_sign_version},
#endif

#if CONFIG_SECURITY_OTA && CONFIG_TFM_FLASH_NSC
	{"ota_brick_test", NULL, ota_brick_command},
#endif/*CONFIG_SECURITY_OTA*/
};

#if (CONFIG_TFM_FWU)
extern int32_t ns_interface_lock_init(void);
#endif
int cli_ota_init(void)
{
#if (CONFIG_TFM_FWU)
	ns_interface_lock_init();
#endif
	return cli_register_commands(s_ota_commands, OTA_CMD_CNT);
}
