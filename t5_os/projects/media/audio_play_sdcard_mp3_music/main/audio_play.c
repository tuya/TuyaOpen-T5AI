// Copyright 2023-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stdio.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <modules/pm.h>
#include <modules/mp3dec.h>
#include "ff.h"
#include "diskio.h"
#include "aud_intf.h"
#include "aud_intf_types.h"
#include "audio_play.h"


#define TAG  "AUD_PLAY_SDCARD_MP3"

#define PCM_SIZE_MAX		(MAX_NSAMP * MAX_NCHAN * MAX_NGRAN)


typedef struct {
    HMP3Decoder hMP3Decoder;
    MP3FrameInfo mp3FrameInfo;
    unsigned char *readBuf;
    short *pcmBuf;
    int bytesLeft;

    FIL mp3file;
    char mp3_file_name[50];
    unsigned char *g_readptr;

    bool mp3_file_is_empty;
} audio_play_info_t;


static audio_play_info_t *audio_play_info = NULL;
static FATFS *pfs = NULL;


static bk_err_t tf_mount(void)
{
	FRESULT fr;

	if (pfs != NULL)
	{
		os_free(pfs);
	}

	pfs = os_malloc(sizeof(FATFS));
	if(NULL == pfs)
	{
		BK_LOGI(TAG, "f_mount malloc failed!\r\n");
		return BK_FAIL;
	}

	fr = f_mount(pfs, "1:", 1);
	if (fr != FR_OK)
	{
		BK_LOGE(TAG, "f_mount failed:%d\r\n", fr);
		return BK_FAIL;
	}
	else
	{
		BK_LOGI(TAG, "f_mount OK!\r\n");
	}

	return BK_OK;
}

static bk_err_t tf_unmount(void)
{
	FRESULT fr;
	fr = f_unmount(DISK_NUMBER_SDIO_SD, "1:", 1);
	if (fr != FR_OK)
	{
		BK_LOGE(TAG, "f_unmount failed:%d\r\n", fr);
		return BK_FAIL;
	}
	else
	{
		BK_LOGI(TAG, "f_unmount OK!\r\n");
	}

	if (pfs)
	{
		os_free(pfs);
		pfs = NULL;
	}

	return BK_OK;
}


static bk_err_t mp3_decode_handler(unsigned int size)
{
	bk_err_t ret = BK_OK;

	FRESULT fr;
	uint32 uiTemp = 0;
    static bool empty_already_flag = false;

    if (!audio_play_info) {
        return BK_FAIL;
    }

	if (audio_play_info->mp3_file_is_empty) {
        if (empty_already_flag == false) {
            empty_already_flag = true;
		    BK_LOGW(TAG, "==========================================================\n");
		    BK_LOGW(TAG, "%s playback is over, please input the stop command!\n", audio_play_info->mp3_file_name);
		    BK_LOGW(TAG, "==========================================================\n");
        }
		return BK_FAIL;
	}

    empty_already_flag = false;

	if (audio_play_info->bytesLeft < MAINBUF_SIZE) {
		os_memmove(audio_play_info->readBuf, audio_play_info->g_readptr, audio_play_info->bytesLeft);
		fr = f_read(&audio_play_info->mp3file, (void *)(audio_play_info->readBuf + audio_play_info->bytesLeft), MAINBUF_SIZE - audio_play_info->bytesLeft, &uiTemp);
		if (fr != FR_OK) {
			BK_LOGE(TAG, "read %s failed\n", audio_play_info->mp3_file_name);
			return fr;
		}

		if ((uiTemp == 0) && (audio_play_info->bytesLeft == 0)) {
			BK_LOGI(TAG, "uiTemp = 0 and bytesLeft = 0\n");
			audio_play_info->mp3_file_is_empty = true;
			BK_LOGI(TAG, "the %s is empty\n", audio_play_info->mp3_file_name);
			return ret;
		}

		audio_play_info->bytesLeft = audio_play_info->bytesLeft + uiTemp;
		audio_play_info->g_readptr = audio_play_info->readBuf;
	}

	int offset = MP3FindSyncWord(audio_play_info->g_readptr, audio_play_info->bytesLeft);

	if (offset < 0) {
		BK_LOGE(TAG, "MP3FindSyncWord not find\n");
		audio_play_info->bytesLeft = 0;
	} else {
		audio_play_info->g_readptr += offset;
		audio_play_info->bytesLeft -= offset;
		
		ret = MP3Decode(audio_play_info->hMP3Decoder, &audio_play_info->g_readptr, &audio_play_info->bytesLeft, audio_play_info->pcmBuf, 0);
		if (ret != ERR_MP3_NONE) {
			BK_LOGE(TAG, "MP3Decode failed, code is %d\n", ret);
			return ret;
		}

		MP3GetLastFrameInfo(audio_play_info->hMP3Decoder, &audio_play_info->mp3FrameInfo);
//		os_printf("Bitrate: %d kb/s, Samprate: %d\r\n", (mp3FrameInfo.bitrate) / 1000, mp3FrameInfo.samprate);
//		os_printf("Channel: %d, Version: %d, Layer: %d\r\n", mp3FrameInfo.nChans, mp3FrameInfo.version, mp3FrameInfo.layer);
//		os_printf("OutputSamps: %d\r\n", mp3FrameInfo.outputSamps);

		/* write a frame speaker data to speaker_ring_buff */
		ret = bk_aud_intf_write_spk_data((uint8_t*)audio_play_info->pcmBuf, audio_play_info->mp3FrameInfo.outputSamps * 2);
		if (ret != BK_OK) {
			BK_LOGE(TAG, "write spk data fail \r\n");
			return ret;
		}
	}

	return ret;
}

bk_err_t audio_play_sdcard_mp3_music_stop(void)
{
	bk_err_t ret;

    if (!audio_play_info) {
        return BK_OK;
    }

	ret = bk_aud_intf_spk_stop();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_spk_stop fail, ret:%d\n", ret);
	}
	
	ret = bk_aud_intf_spk_deinit();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_spk_deinit fail, ret:%d\n", ret);
	}

    ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_set_mode fail, ret:%d\n", ret);
	}

	ret = bk_aud_intf_drv_deinit();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_drv_deinit fail, ret:%d\n", ret);
	}

	audio_play_info->bytesLeft = 0;
	audio_play_info->mp3_file_is_empty = false;

	f_close(&audio_play_info->mp3file);

    if (audio_play_info->hMP3Decoder) {
	    MP3FreeDecoder(audio_play_info->hMP3Decoder);
        audio_play_info->hMP3Decoder = NULL;
    }

    if (audio_play_info->readBuf) {
        os_free(audio_play_info->readBuf);
        audio_play_info->readBuf = NULL;
    }

    if (audio_play_info->pcmBuf) {
        os_free(audio_play_info->pcmBuf);
        audio_play_info->pcmBuf = NULL;
    }

    if (audio_play_info) {
        os_free(audio_play_info);
        audio_play_info = NULL;
    }

    tf_unmount();
    tf_unmount();

    return BK_OK;
}

bk_err_t audio_play_sdcard_mp3_music_start(char *file_name)
{
	bk_err_t ret = BK_OK;
    uint32 uiTemp = 0;
	char tag_header[10];
	int tag_size = 0;

	aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
	aud_intf_spk_setup_t aud_intf_spk_setup = DEFAULT_AUD_INTF_SPK_SETUP_CONFIG();

	if (!file_name) {
		BK_LOGE(TAG, "file_name is NULL\n");
		return BK_FAIL;
	}

    ret = tf_mount();
    if (ret != BK_OK) {
        BK_LOGE(TAG, "mount sdcard fail\n");
        return BK_FAIL;
    }

    audio_play_info = (audio_play_info_t *)os_malloc(sizeof(audio_play_info_t));
    if (!audio_play_info) {
        BK_LOGE(TAG, "mount sdcard fail\n");
        goto fail;
    }

    os_memset(audio_play_info, 0, sizeof(audio_play_info_t));

	audio_play_info->readBuf = os_malloc(MAINBUF_SIZE);
	if (audio_play_info->readBuf == NULL) {
		BK_LOGE(TAG, "readBuf malloc fail\n");
		goto fail;
	}
    os_memset(audio_play_info->readBuf, 0, MAINBUF_SIZE);

	audio_play_info->pcmBuf = os_malloc(PCM_SIZE_MAX * 2);
	if (audio_play_info->pcmBuf == NULL) {
		BK_LOGE(TAG, "pcmBuf malloc fail\n");
		goto fail;
	}
    os_memset(audio_play_info->pcmBuf, 0, PCM_SIZE_MAX * 2);

	audio_play_info->hMP3Decoder = MP3InitDecoder();
	if (audio_play_info->hMP3Decoder == NULL) {
		BK_LOGE(TAG, "MP3Decoder init fail\n");
		goto fail;
	}

	BK_LOGI(TAG, "audio mp3 play decode init complete\n");

	/*open file to read mp3 data */
    os_memset(audio_play_info->mp3_file_name, 0, sizeof(audio_play_info->mp3_file_name)/sizeof(audio_play_info->mp3_file_name[0]));
	sprintf(audio_play_info->mp3_file_name, "%d:/%s", DISK_NUMBER_SDIO_SD, file_name);
	FRESULT fr = f_open(&audio_play_info->mp3file, audio_play_info->mp3_file_name, FA_OPEN_EXISTING | FA_READ);
	if (fr != FR_OK) {
		BK_LOGE(TAG, "open %s fail\n", audio_play_info->mp3_file_name);
		goto fail;
	}
	BK_LOGI(TAG, "mp3 file: %s open successful\n", audio_play_info->mp3_file_name);

    fr = f_read(&audio_play_info->mp3file, (void *)tag_header, 10, &uiTemp);
    if (fr != FR_OK)
    {
        BK_LOGE(TAG, "read %s fail\n", audio_play_info->mp3_file_name);
        goto fail;
    }

    if (os_memcmp(tag_header, "ID3", 3) == 0)
    {
        tag_size = ((tag_header[6] & 0x7F) << 21) | ((tag_header[7] & 0x7F) << 14) | ((tag_header[8] & 0x7F) << 7) | (tag_header[9] & 0x7F);
        BK_LOGI(TAG, "tag_size = %d\n", tag_size);
        f_lseek(&audio_play_info->mp3file, tag_size + 10);
        BK_LOGI(TAG, "tag_header has found\n");
    }
    else
    {
        BK_LOGI(TAG, "tag_header not found\n");
        f_lseek(&audio_play_info->mp3file, 0);
    }

	//aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
	//aud_intf_drv_setup.task_config.priority = 3;
	aud_intf_drv_setup.aud_intf_rx_spk_data = mp3_decode_handler;
	//aud_intf_drv_setup.aud_intf_tx_mic_data = NULL;
	ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
        goto fail;
	}

	ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_GENERAL);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
        goto fail;
	}

	audio_play_info->g_readptr = audio_play_info->readBuf;
	ret = mp3_decode_handler(audio_play_info->mp3FrameInfo.outputSamps * 2);
    if (ret < 0) {
        BK_LOGE(TAG, "mp3_decode_handler fail, ret:%d\n", ret);
        goto fail;
    }

    switch (audio_play_info->mp3FrameInfo.nChans)
    {
        case 1:
            aud_intf_spk_setup.spk_chl = AUD_INTF_SPK_CHL_LEFT;
            break;

        case 2:
            aud_intf_spk_setup.spk_chl = AUD_INTF_SPK_CHL_DUAL;
            break;

        default:
            BK_LOGE(TAG, "nChans:%d is not support\n", audio_play_info->mp3FrameInfo.nChans);
            goto fail;
            break;
    }
	aud_intf_spk_setup.samp_rate = audio_play_info->mp3FrameInfo.samprate;
	aud_intf_spk_setup.frame_size = audio_play_info->mp3FrameInfo.outputSamps * 2;
	aud_intf_spk_setup.spk_gain = 0x20;
	aud_intf_spk_setup.work_mode = AUD_DAC_WORK_MODE_DIFFEN;
	//aud_intf_spk_setup.spk_type = AUD_INTF_SPK_TYPE_UAC;
	ret = bk_aud_intf_spk_init(&aud_intf_spk_setup);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_spk_init fail, ret:%d\n", ret);
        goto fail;
	}

	ret = bk_aud_intf_spk_start();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_spk_start fail, ret:%d\n", ret);
        goto fail;
	}

    return BK_OK;

fail:

    audio_play_sdcard_mp3_music_stop();

	return BK_FAIL;
}

