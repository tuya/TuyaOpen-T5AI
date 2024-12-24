// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cli.h"

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdio.h>
#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>
#include <driver/dma.h>
#include "sys_driver.h"
//#include "aud_driver.h"
// #include "mp3_demo_types.h"
#include <driver/audio_ring_buff.h>
#include <modules/mp3dec.h>
// #include "ff.h"
// #include "diskio.h"
#include "aud_intf.h"
#include "aud_intf_types.h"

#include "tkl_audio.h"
#include "tkl_gpio.h"
#include "tkl_fs.h"

#include "cli_tuya_test.h"


#define TU_QITEM_COUNT      (60)
#define PCM_SIZE_MAX		(MAX_NSAMP * MAX_NCHAN * MAX_NGRAN)

HMP3Decoder hMP3Decoder;
MP3FrameInfo mp3FrameInfo;
unsigned char *readBuf;
short *pcmBuf;
int bytesLeft = 0;

TUYA_FILE mp3file;
TUYA_FILE pcmfile;

int offset = 0;
unsigned char *g_readptr;

static bool mp3_file_is_empty = false;
#define MP3_DATA_BUF_SIZE   1940

static bk_err_t mp3_decode_handler(unsigned int size)
{
    bk_err_t ret = BK_OK;

    uint32 uiTemp = 0;

    if (mp3_file_is_empty) {
        os_printf("==========================================================\r\n");
        os_printf("playback is over, please input the stop command!\r\n");
        os_printf("==========================================================\r\n");
        return -100;
    }

    if (bytesLeft < MP3_DATA_BUF_SIZE) {
        os_memmove(readBuf, g_readptr, bytesLeft);

        uiTemp = tkl_fread((void *)(readBuf + bytesLeft), MP3_DATA_BUF_SIZE - bytesLeft, mp3file);
        if (uiTemp == 0) {
            os_printf("uiTemp = 0\r\n");
            mp3_file_is_empty = true;
            os_printf("the test.mp3 is play over\r\n");
            return 0;
        }
        bytesLeft = bytesLeft + uiTemp;
        g_readptr = readBuf;
    }

    offset = MP3FindSyncWord(g_readptr, bytesLeft);

    if (offset < 0) {
        os_printf("MP3FindSyncWord not find!\r\n");
        bytesLeft = 0;
    } else {
        g_readptr += offset;
        bytesLeft -= offset;

        ret = MP3Decode(hMP3Decoder, &g_readptr, &bytesLeft, pcmBuf, 0);
        if (ret != ERR_MP3_NONE) {
            os_printf("MP3Decode failed, code is %d", ret);
            return ret;
        }

        MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
        // os_printf("Bitrate: %d kb/s, Samprate: %d\r\n", (mp3FrameInfo.bitrate) / 1000, mp3FrameInfo.samprate);
        // os_printf("Channel: %d, Version: %d, Layer: %d\r\n", mp3FrameInfo.nChans, mp3FrameInfo.version, mp3FrameInfo.layer);
        // os_printf("OutputSamps: %d\r\n", mp3FrameInfo.outputSamps);

        #if 1
        /* write a frame speaker data to speaker_ring_buff */
        TKL_AUDIO_FRAME_INFO_T frame;
        frame.pbuf = pcmBuf;
        frame.used_size = mp3FrameInfo.outputSamps * 2;
        tkl_ao_put_frame(0, 0, NULL, &frame);
        #else
        int i = 0;
        uint32_t write_len = mp3FrameInfo.outputSamps * 2;
        for (i = 0; i < write_len / 320; i++) {
write_spk_retry1:
            ret = bk_aud_intf_write_spk_data(((uint8_t*)pcmBuf) + i * 320, 320);
            if (ret == BK_ERR_BUSY) {
                tkl_system_sleep(10);
                goto write_spk_retry1;
            }
        }

        if ((write_len % 320) != 0) {
write_spk_retry2:
            ret = bk_aud_intf_write_spk_data(((uint8_t*)pcmBuf) + i * 320, write_len % 320);
            if (ret == BK_ERR_BUSY) {
                tkl_system_sleep(10);
                goto write_spk_retry2;
            }
        }
        #endif
    }
    return ret;
}

void bk_audio_mp3_play_decode_init(void)
{
    readBuf = os_malloc(MP3_DATA_BUF_SIZE);
    if (readBuf == NULL) {
        os_printf("readBuf malloc failed!\r\n");
        return;
    }

    pcmBuf = os_malloc(PCM_SIZE_MAX * 2);
    if (pcmBuf == NULL) {
        os_printf("pcmBuf malloc failed!\r\n");
        return;
    }

    hMP3Decoder = MP3InitDecoder();
    if (hMP3Decoder == 0) {
        os_free(readBuf);
        os_free(pcmBuf);
        os_printf("MP3Decoder init failed!\r\n");
        return;
    }

}

static int cli_mp3_test_init(const char *f)
{
    bk_audio_mp3_play_decode_init();
    os_printf("audio mp3 play decode init completely!\r\n");
    /*open file to read mp3 data */
    // mp3file = tkl_fopen("/stereo.mp3", "r");
    mp3file = tkl_fopen(f, "r");
    if (mp3file == NULL) {
        os_printf("mp3 file open failed!\r\n");
        return -1;
    }
    os_printf("mp3 file %s open successfully!\r\n", f);
    g_readptr = readBuf;
    return 0;
}

static int __test_ai_cb(TKL_AUDIO_FRAME_INFO_T *pframe)
{
//     bk_printf("%s %d: %p\r\n", __func__, __LINE__, pframe);
//     for (int i = 0; i < 16; i++)
//         bk_printf("%02x ", (uint8_t)pframe->pbuf[i]);
//     bk_printf("\r\n");

    return pframe->buf_size;
}

void cli_aud_intf_mp3_play_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret = 0;
    char *mf = NULL;
    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("%s ", argv[i]);
    }
    bk_printf("\r\n");

    if (argv[2] == NULL) {
        mf = "/test.mp3";
    } else {
        mf = argv[2];
    }
    // 1, mp3 init
    ret = cli_mp3_test_init(mf);
    if (ret < 0) {
        return;
    }

    os_printf("audio trace: %s %d\r\n", __func__, __LINE__);
    TKL_AUDIO_CONFIG_T config;

    // 2, mic init
    config.enable = 0;
    config.ai_chn = 0;
    config.sample = 8000;
    config.spk_sample = 24000;
    config.datebits = 16;
    config.channel = 1;
    config.codectype = TKL_CODEC_AUDIO_PCM;
    config.card = TKL_AUDIO_TYPE_BOARD;
    config.put_cb = __test_ai_cb;
    config.spk_gpio = 27;
    config.spk_gpio_polarity = 0;

    tkl_ai_init(&config, 0);
    os_printf("audio trace: %s %d\r\n", __func__, __LINE__);

    tkl_ai_start(0, 0);
    os_printf("audio trace: %s %d\r\n", __func__, __LINE__);

    tkl_ai_set_vol(0, 0, 80);

    extern void tkl_system_sleep(uint32_t num_ms);
    tkl_system_sleep(200);
    do {
        ret = mp3_decode_handler(0);
        if (ret == -100) {
            break;
        }
        tkl_system_sleep(20);
    } while (ret <= 0);


    // stop && deinit
    tkl_ai_stop(TKL_AUDIO_TYPE_BOARD, 0);

    tkl_ai_uninit();

    bytesLeft = 0;
    mp3_file_is_empty = false;

    tkl_fclose(mp3file);

    MP3FreeDecoder(hMP3Decoder);
    os_free(readBuf); readBuf = NULL;
    os_free(pcmBuf); pcmBuf = NULL;
}

#define MP3_CMD_CNT (sizeof(s_mp3_commands) / sizeof(struct cli_command))
static const struct cli_command s_mp3_commands[] = {
    {"xmp3", "aud_intf_mp3_play_test {start|stop} xx.mp3", cli_aud_intf_mp3_play_test_cmd},
};

int cli_mp3_init(void)
{
    return cli_register_commands(s_mp3_commands, MP3_CMD_CNT);
}

