/**
 * @file lv_port_disp.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "os/os.h"
#include "os/mem.h"
#include "lv_vendor.h"
#include <frame_buffer.h>
#include "yuv_encode.h"
#include "driver/dma2d.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "LVGL_DISP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_deinit(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
frame_buffer_t *lvgl_frame_buffer = NULL;
uint8_t lvgl_camera_switch_flag = 0;
extern lv_vnd_config_t vendor_config;
extern media_debug_t *media_debug;

static beken_semaphore_t lv_dma2d_sem = NULL;
static uint8_t lv_dma2d_use_flag = 0;


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(vendor_config.lcd_hor_res, vendor_config.lcd_ver_res);
    lv_display_set_flush_cb(disp, disp_flush);

    #if 0
    /* Example 1
     * One buffer for partial rendering*/
    static lv_color_t buf_1_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
    lv_display_set_buffers(disp, buf_1_1, NULL, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];
    static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];
    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];
    lv_display_set_buffers(disp, buf_3_1, buf_3_2, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);
    #else
    #if LVGL_USE_PSRAM
    lv_display_set_buffers(disp, vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2, vendor_config.draw_pixel_size, LV_DISPLAY_RENDER_MODE_DIRECT);
    #else
    lv_display_set_buffers(disp, vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2, vendor_config.draw_pixel_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    #endif
    #endif

    LOGI("LVGL addr1:%x, addr2:%x, pixel size:%d, fb1:%x, fb2:%x\r\n", vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2,
                                            vendor_config.draw_pixel_size, vendor_config.frame_buf_1, vendor_config.frame_buf_2);
}

void lv_port_disp_deinit(void)
{
    lv_display_delete(lv_disp_get_default());

    disp_deinit();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_memcpy_one_line(void *dest_buf, const void *src_buf, uint32_t point_num)
{
    #if (LV_COLOR_DEPTH == 16)
        os_memcpy(dest_buf, src_buf, point_num * sizeof(lv_color16_t));
    #elif (LV_COLOR_DEPTH == 24)
        os_memcpy(dest_buf, src_buf, point_num * sizeof(lv_color_t));
    #endif
}

static void lv_dma2d_config_error(void)
{
    LOGE("%s \n", __func__);
}

static void lv_dma2d_transfer_error(void)
{
    LOGE("%s \n", __func__);
}

static void lv_dma2d_transfer_complete(void)
{
    rtos_set_semaphore(&lv_dma2d_sem);
}

static void lv_dma2d_memcpy_init(void)
{
    bk_err_t ret;

    ret = rtos_init_semaphore_ex(&lv_dma2d_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s %d lv_dma2d_sem init failed\n", __func__, __LINE__);
        return;
    }

    bk_dma2d_driver_init();
    bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, lv_dma2d_config_error);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, lv_dma2d_transfer_error);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, lv_dma2d_transfer_complete);
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 1);
}

static void lv_dma2d_memcpy_deinit(void)
{
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 0);
    bk_dma2d_driver_deinit();
    rtos_deinit_semaphore(&lv_dma2d_sem);
}

static void lv_dma2d_memcpy(void *Psrc, uint32_t src_xsize, uint32_t src_ysize,
                                    void *Pdst, uint32_t dst_xsize, uint32_t dst_ysize,
                                    uint32_t dst_xpos, uint32_t dst_ypos)
{
    dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};

    dma2d_memcpy_pfc.input_addr = (char *)Psrc;
    dma2d_memcpy_pfc.output_addr = (char *)Pdst;

    #if (LV_COLOR_DEPTH == 16)
    dma2d_memcpy_pfc.mode = DMA2D_M2M;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_RGB565;
    dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
    dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
    #elif (LV_COLOR_DEPTH == 24)
    dma2d_memcpy_pfc.mode = DMA2D_M2M;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_RGB888;
    dma2d_memcpy_pfc.src_pixel_byte = THREE_BYTES;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB888;
    dma2d_memcpy_pfc.dst_pixel_byte = THREE_BYTES;
    #elif (LV_COLOR_DEPTH == 32)
    dma2d_memcpy_pfc.mode = DMA2D_M2M;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_ARGB8888;
    dma2d_memcpy_pfc.src_pixel_byte = FOUR_BYTES;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_ARGB8888;
    dma2d_memcpy_pfc.dst_pixel_byte = FOUR_BYTES;
    #endif

    dma2d_memcpy_pfc.dma2d_width = src_xsize;
    dma2d_memcpy_pfc.dma2d_height = src_ysize;
    dma2d_memcpy_pfc.src_frame_width = src_xsize;
    dma2d_memcpy_pfc.src_frame_height = src_ysize;
    dma2d_memcpy_pfc.dst_frame_width = dst_xsize;
    dma2d_memcpy_pfc.dst_frame_height = dst_ysize;
    dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    dma2d_memcpy_pfc.dst_frame_xpos = dst_xpos;
    dma2d_memcpy_pfc.dst_frame_ypos = dst_ypos;

    bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();
}

static void lv_dma2d_memcpy_wait_transfer_finish(void)
{
    bk_err_t ret = BK_OK;

    if (lv_dma2d_sem && lv_dma2d_use_flag) {
        ret = rtos_get_semaphore(&lv_dma2d_sem, 1000);
        if (ret != kNoErr) {
            LOGE("%s lv_dma2d_sem get fail! ret = %d\r\n", __func__, ret);
        }
        lv_dma2d_use_flag = 0;
    }
}

static void lv_dma2d_memcpy_last_frame(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize, uint32_t src_offline, uint32_t dest_offline)
{
    lv_dma2d_memcpy_wait_transfer_finish();
    dma2d_memcpy_psram(Psrc, Pdst, xsize, ysize, src_offline, dest_offline);
    lv_dma2d_use_flag = 1;
}

static void lv_dma2d_memcpy_draw_buffer(void *Psrc, uint32_t src_xsize, uint32_t src_ysize, void *Pdst, uint32_t dst_xpos, uint32_t dst_ypos)
{
    lv_dma2d_memcpy_wait_transfer_finish();
    lv_dma2d_memcpy(Psrc, src_xsize, src_ysize, Pdst, vendor_config.lcd_hor_res, vendor_config.lcd_ver_res, dst_xpos, dst_ypos);
    lv_dma2d_use_flag = 1;
}


static void lvgl_frame_buffer_free(frame_buffer_t *frame_buffer)
{
    //To do
}

const frame_buffer_callback_t lvgl_frame_buffer_cb =
{
    .free = lvgl_frame_buffer_free,
};

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
    if (lv_vendor_display_frame_cnt() == 2 || lv_vendor_draw_buffer_cnt() == 2) {
        lv_dma2d_memcpy_init();
    }

    if (!lvgl_camera_switch_flag) {
// Modified by TUYA Start
        // lvgl_frame_buffer = os_malloc(sizeof(frame_buffer_t));
        lvgl_frame_buffer = LV_MEM_CUSTOM_ALLOC(sizeof(frame_buffer_t));
// Modified by TUYA End
        if (!lvgl_frame_buffer) {
            LOGI("%s %d lvgl_frame_buffer malloc fail\r\n", __func__, __LINE__);
            return;
        }

        os_memset(lvgl_frame_buffer, 0, sizeof(frame_buffer_t));

        lvgl_frame_buffer->width = vendor_config.lcd_hor_res;
        lvgl_frame_buffer->height = vendor_config.lcd_ver_res;

        #if (LV_COLOR_DEPTH == 16)
        lvgl_frame_buffer->fmt = PIXEL_FMT_RGB565;
        #elif (LV_COLOR_DEPTH == 24)
        lvgl_frame_buffer->fmt = PIXEL_FMT_RGB888;
        #endif

        lvgl_frame_buffer->cb = &lvgl_frame_buffer_cb;
    }
}

static void disp_deinit(void)
{
    if (lv_vendor_display_frame_cnt() == 2 || lv_vendor_draw_buffer_cnt() == 2) {
        lv_dma2d_memcpy_deinit();
    }

    if (lvgl_frame_buffer) {
// Modified by TUYA Start
        // os_free(lvgl_frame_buffer);
        LV_MEM_CUSTOM_FREE(lvgl_frame_buffer);
// Modified by TUYA End
        lvgl_frame_buffer = NULL;
    }
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    if (disp_flush_enabled) {
    #if (!LVGL_USE_PSRAM)
        lv_color16_t *color_ptr = (lv_color16_t *)px_map;
        static lv_color16_t *disp_buf = NULL;
        static u8 disp_buff_index = DISPLAY_BUFFER_DEF;
        lv_color16_t *disp_buf1 = vendor_config.frame_buf_1;
        lv_color16_t *disp_buf2 = vendor_config.frame_buf_2;

        lv_coord_t lv_hor = LV_HOR_RES;
        lv_coord_t lv_ver = LV_VER_RES;

        int y = 0;
        int offset = 0;
        lv_coord_t width = lv_area_get_width(area);
        lv_coord_t height = lv_area_get_height(area);

        if (disp_buf2 != NULL) {
            if(DISPLAY_BUFFER_1 == disp_buff_index) {
                lv_dma2d_memcpy_wait_transfer_finish();
                disp_buf = disp_buf2;
            } else if (DISPLAY_BUFFER_2 == disp_buff_index) {
                lv_dma2d_memcpy_wait_transfer_finish();
                disp_buf = disp_buf1;
            }
            else //first display
            {
                lv_dma2d_memcpy_wait_transfer_finish();
                disp_buf = disp_buf1;
            }
        } else {
            disp_buf = disp_buf1;
        }

        if (ROTATE_NONE == vendor_config.rotation) {
            if (lv_vendor_draw_buffer_cnt() == 2) {
                lv_dma2d_memcpy_draw_buffer(color_ptr, width, height, disp_buf, area->x1, area->y1);
            } else {
                offset = area->y1 * lv_hor + area->x1;
                for (y = area->y1; y <= area->y2 && y < lv_ver; y++) {
                    lv_memcpy_one_line(disp_buf + offset, color_ptr, width);
                    offset += lv_hor;
                    color_ptr += width;
                }
            }
        }

        if (lv_display_flush_is_last(disp)) {
            media_debug->lvgl_draw++;
            if (!lvgl_camera_switch_flag) {
                lvgl_frame_buffer->frame = (uint8_t *)disp_buf;
            }

            if (lv_vendor_draw_buffer_cnt() == 2) {
                lv_dma2d_memcpy_wait_transfer_finish();
            }

            lcd_display_frame_request(lvgl_frame_buffer);

            if (disp_buf2) {
                if (DISPLAY_BUFFER_1 == disp_buff_index) {
                    lv_dma2d_memcpy_last_frame(disp_buf, disp_buf1, lv_hor, lv_ver, 0, 0);
                    disp_buff_index = 2;
                } else if (DISPLAY_BUFFER_2 == disp_buff_index) {
                    lv_dma2d_memcpy_last_frame(disp_buf, disp_buf2, lv_hor, lv_ver, 0, 0);
                    disp_buff_index = 1;
                }
                else //first display
                {
                    lv_dma2d_memcpy_last_frame(disp_buf, disp_buf2, lv_hor, lv_ver, 0, 0);
                    disp_buff_index = 2;
                }
            }
        }
    #else
        media_debug->lvgl_draw++;
        lvgl_frame_buffer->frame = (uint8_t *)px_map;
        lcd_display_frame_request(lvgl_frame_buffer);
    #endif
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_display_flush_ready(disp);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
