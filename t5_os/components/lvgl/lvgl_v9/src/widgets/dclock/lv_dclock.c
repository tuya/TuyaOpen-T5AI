/**
 * @file lv_dclock.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_dclock.h"
#if LV_USE_DCLOCK != 0
#include "../../../core/lv_obj.h"
#include "../../../misc/lv_assert.h"
#include "../../../core/lv_group.h"
#include "../../../draw/lv_draw.h"
#include "../../../misc/lv_color.h"
#include "../../../misc/lv_math.h"
#include "../../../misc/lv_bidi.h"
#include "../../../misc/lv_txt_ap.h"
#include "../../../misc/lv_printf.h"
#include "../../../widgets/lv_label.h"


/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_dclock_class

#define LV_DCLOCK_DEF_SCROLL_SPEED   (lv_disp_get_dpi(lv_obj_get_disp(obj)) / 3)
#define LV_DCLOCK_SCROLL_DELAY       300
#define LV_DCLOCK_DOT_END_INV 0xFFFFFFFF
#define LV_DCLOCK_HINT_HEIGHT_LIMIT 1024 /*Enable "hint" to buffer info about labels larger than this. (Speed up drawing)*/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_dclock_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_dclock_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_dclock_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void draw_main(lv_event_t * e);
static void lv_dclock_refr_text(lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_dclock_class = {
    .constructor_cb = lv_dclock_constructor,
    .destructor_cb = lv_dclock_destructor,
    .event_cb = lv_dclock_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_dclock_t),
    .base_class = &lv_obj_class
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_dclock_create(lv_obj_t * parent, char * input_time)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    if(lv_obj_is_valid(obj)) {
        lv_dclock_set_text_fmt(obj, "%s", input_time);
    }
    return obj;
}

void lv_dclock_set_text(lv_obj_t * obj, const char * text)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_dclock_t * dclock = (lv_dclock_t *)obj;

    lv_obj_invalidate(obj);

    /*If text is NULL then just refresh with the current text*/
    if(text == NULL) text = dclock->text;

    if(dclock->text == text && dclock->static_txt == 0) {
        /*If set its own text then reallocate it (maybe its size changed)*/
#if LV_USE_ARABIC_PERSIAN_CHARS
        /*Get the size of the text and process it*/
        size_t len = _lv_txt_ap_calc_bytes_cnt(text);

        dclock->text = lv_mem_realloc(dclock->text, len);
        LV_ASSERT_MALLOC(dclock->text);
        if(dclock->text == NULL) return;

        _lv_txt_ap_proc(dclock->text, dclock->text);
#else
        dclock->text = lv_mem_realloc(dclock->text, strlen(dclock->text) + 1);
#endif

        LV_ASSERT_MALLOC(dclock->text);
        if(dclock->text == NULL) return;
    }
    else {
        /*Free the old text*/
        if(dclock->text != NULL && dclock->static_txt == 0) {
            lv_mem_free(dclock->text);
            dclock->text = NULL;
        }

#if LV_USE_ARABIC_PERSIAN_CHARS
        /*Get the size of the text and process it*/
        size_t len = _lv_txt_ap_calc_bytes_cnt(text);

        dclock->text = lv_mem_alloc(len);
        LV_ASSERT_MALLOC(dclock->text);
        if(dclock->text == NULL) return;

        _lv_txt_ap_proc(text, dclock->text);
#else
        /*Get the size of the text*/
        size_t len = strlen(text) + 1;

        /*Allocate space for the new text*/
        dclock->text = lv_mem_alloc(len);
        LV_ASSERT_MALLOC(dclock->text);
        if(dclock->text == NULL) return;
        strcpy(dclock->text, text);
#endif

        /*Now the text is dynamically allocated*/
        dclock->static_txt = 0;
    }

    lv_dclock_refr_text(obj);
}

void lv_dclock_set_text_fmt(lv_obj_t * obj, const char * fmt, ...)
{

    LV_ASSERT_OBJ(obj, MY_CLASS);
    LV_ASSERT_NULL(fmt);
    lv_obj_invalidate(obj);
    lv_dclock_t * dclock = (lv_dclock_t *)obj;

    /*If text is NULL then refresh*/
    if(fmt == NULL) {
        lv_dclock_refr_text(obj);
        return;
    }

    if(dclock->text != NULL && dclock->static_txt == 0) {
        lv_mem_free(dclock->text);
        dclock->text = NULL;
    }

    va_list args;
    va_start(args, fmt);
    dclock->text = _lv_txt_set_text_vfmt(fmt, args);
    va_end(args);
    dclock->static_txt = 0; /*Now the text is dynamically allocated*/

    lv_dclock_refr_text(obj);
}

/*=====================
 * Getter functions
 *====================*/

uint32_t lv_dclock_get_text_selection_start(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

#if LV_DCLOCK_TEXT_SELECTION
    lv_dclock_t * dclock = (lv_dclock_t *)obj;
    return dclock->sel_start;

#else
    LV_UNUSED(obj); /*Unused*/
    return LV_DCLOCK_TEXT_SELECTION_OFF;
#endif
}

uint32_t lv_dclock_get_text_selection_end(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

#if LV_DCLOCK_TEXT_SELECTION
    lv_dclock_t * dclock = (lv_dclock_t *)obj;
    return dclock->sel_end;
#else
    LV_UNUSED(obj); /*Unused*/
    return LV_DCLOCK_TEXT_SELECTION_OFF;
#endif
}


/*=====================
 * Other functions
 *====================*/

/**
 * @brief           Calculate the digital clock by the 24-hours mode
 * @param obj       pointer to the values for hour/minute/seconds and the bool value to select AM/PM
 * @return          selection end index. The function of lv_dclock_24_timer_cb will call it.
 */
void clock_count_24(int * hour, int * minute, int * seconds)
{
    (*seconds)++;

    if(*seconds == 60) {
        *seconds = 0;
        (*minute)++;
    }

    if(*minute == 60) {
        *minute = 0;
        (*hour)++;
    }

    if(*hour == 24) {
        *hour = 0;
    }
}

/**
 * @brief           Calculate the digital clock by the 12-hours mode
 * @param obj       pointer to the values for hour/minute/seconds and the bool value to select AM/PM
 * @return          selection end index. The function of lv_dclock_12_timer_cb will call it.
 */
void clock_count_12(int * hour, int * minute, int * seconds, char * meridiem)
{

    (*seconds)++;
    if(*seconds == 60) {
        *seconds = 0;
        (*minute)++;
    }
    if(*minute == 60) {
        *minute = 0;
        if(*hour < 12) {
            (*hour)++;
        }
        else {
            (*hour)++;
            (*hour) = (*hour) % 12;
        }
    }
    if(*hour == 12 && *seconds == 0 && *minute == 0) {
        if((strcmp(meridiem, "PM") == 0)) {
            strcpy(meridiem, "AM");
        }
        else {
            strcpy(meridiem, "PM");
        }
    }
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_dclock_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_dclock_t * dclock = (lv_dclock_t *)obj;

    dclock->text       = NULL;
    dclock->static_txt = 0;
    dclock->recolor    = 0;
    dclock->offset.x = 0;
    dclock->offset.y = 0;

#if LV_DCLOCK_TEXT_SELECTION
    dclock->sel_start = LV_DRAW_LABEL_NO_TXT_SEL;
    dclock->sel_end   = LV_DRAW_LABEL_NO_TXT_SEL;
#endif

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_dclock_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_dclock_t * dclock = (lv_dclock_t *)obj;

    if(!dclock->static_txt) lv_mem_free(dclock->text);
    dclock->text = NULL;
}

static void lv_dclock_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_STYLE_CHANGED) {
        lv_dclock_refr_text(obj);
    }
    else if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        /* Italic or other non-typical letters can be drawn of out of the object.
         * It happens if box_w + ofs_x > adw_w in the glyph.
         * To avoid this add some extra draw area.
         * font_h / 4 is an empirical value. */
        const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_coord_t font_h = lv_font_get_line_height(font);
        lv_event_set_ext_draw_size(e, font_h / 4);
    }
    else if(code == LV_EVENT_SIZE_CHANGED) {
        lv_dclock_refr_text(obj);
    }
    else if(code == LV_EVENT_GET_SELF_SIZE) {
        lv_point_t size;
        lv_dclock_t * dclock = (lv_dclock_t *)obj;
        const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_coord_t letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);
        lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
        lv_text_flag_t flag = LV_TEXT_FLAG_NONE;
        if(dclock->recolor != 0) flag |= LV_TEXT_FLAG_RECOLOR;
        if(dclock->expand != 0) flag |= LV_TEXT_FLAG_EXPAND;

        lv_coord_t w = lv_obj_get_content_width(obj);
        if(lv_obj_get_style_width(obj, LV_PART_MAIN) == LV_SIZE_CONTENT && !obj->w_layout) w = LV_COORD_MAX;
        else w = lv_obj_get_content_width(obj);

        lv_txt_get_size(&size, dclock->text, font, letter_space, line_space, w, flag);

        lv_point_t * self_size = lv_event_get_param(e);
        self_size->x = LV_MAX(self_size->x, size.x);
        self_size->y = LV_MAX(self_size->y, size.y);
    }
    else if(code == LV_EVENT_DRAW_MAIN) {
        draw_main(e);
    }
}

static void draw_main(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_dclock_t * dclock = (lv_dclock_t *)obj;
    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);

    lv_area_t txt_coords;
    lv_obj_get_content_coords(obj, &txt_coords);

    lv_text_flag_t flag = LV_TEXT_FLAG_NONE;
    if(dclock->recolor != 0) flag |= LV_TEXT_FLAG_RECOLOR;
    if(dclock->expand != 0) flag |= LV_TEXT_FLAG_EXPAND;
    if(lv_obj_get_style_width(obj, LV_PART_MAIN) == LV_SIZE_CONTENT && !obj->w_layout) flag |= LV_TEXT_FLAG_FIT;

    lv_draw_label_dsc_t dclock_draw_dsc;
    lv_draw_label_dsc_init(&dclock_draw_dsc);

    dclock_draw_dsc.ofs_x = dclock->offset.x;
    dclock_draw_dsc.ofs_y = dclock->offset.y;

    dclock_draw_dsc.flag = flag;
    lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &dclock_draw_dsc);
    lv_bidi_calculate_align(&dclock_draw_dsc.align, &dclock_draw_dsc.bidi_dir, dclock->text);

    dclock_draw_dsc.sel_start = lv_dclock_get_text_selection_start(obj);
    dclock_draw_dsc.sel_end = lv_dclock_get_text_selection_end(obj);
    if(dclock_draw_dsc.sel_start != LV_DRAW_LABEL_NO_TXT_SEL && dclock_draw_dsc.sel_end != LV_DRAW_LABEL_NO_TXT_SEL) {
        dclock_draw_dsc.sel_color = lv_obj_get_style_text_color_filtered(obj, LV_PART_SELECTED);
        dclock_draw_dsc.sel_bg_color = lv_obj_get_style_bg_color(obj, LV_PART_SELECTED);
    }

    /* In SCROLL and SCROLL_CIRCULAR mode the CENTER and RIGHT are pointless, so remove them.
     * (In addition, they will create misalignment in this situation)*/

#if LV_DCLOCK_LONG_TXT_HINT
    lv_draw_label_hint_t * hint = &dclock->hint;
    if(dclock->long_mode == LV_DCLOCK_LONG_SCROLL_CIRCULAR || lv_area_get_height(&txt_coords) < LV_DCLOCK_HINT_HEIGHT_LIMIT)
        hint = NULL;

#else
    /*Just for compatibility*/
    lv_draw_label_hint_t * hint = NULL;
#endif

    lv_area_t txt_clip;
    bool is_common = _lv_area_intersect(&txt_clip, &txt_coords, draw_ctx->clip_area);
    if(!is_common) return;

    lv_draw_label(draw_ctx, &dclock_draw_dsc, &txt_coords, dclock->text, hint); /// copy the above case
    const lv_area_t * clip_area_ori = draw_ctx->clip_area;
    draw_ctx->clip_area = &txt_clip;

    draw_ctx->clip_area = clip_area_ori;
}

/**
 * Refresh the dclock with its text stored in its extended data
 * @param dclock pointer to a label object
 */
static void lv_dclock_refr_text(lv_obj_t * obj)
{

    lv_dclock_t * dclock = (lv_dclock_t *)obj;
    if(dclock->text == NULL) return;
#if LV_DCLOCK_LONG_TXT_HINT
    dclock->hint.line_start = -1; /*The hint is invalid if the text changes*/
#endif

    lv_area_t txt_coords;
    lv_obj_get_content_coords(obj, &txt_coords);
    lv_coord_t max_w         = lv_area_get_width(&txt_coords);
    const lv_font_t * font   = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
    lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
    lv_coord_t letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);

    /*Calc. the height and longest line*/
    lv_point_t size;
    lv_text_flag_t flag = LV_TEXT_FLAG_NONE;
    if(dclock->recolor != 0) flag |= LV_TEXT_FLAG_RECOLOR;
    if(dclock->expand != 0) flag |= LV_TEXT_FLAG_EXPAND;
    if(lv_obj_get_style_width(obj, LV_PART_MAIN) == LV_SIZE_CONTENT && !obj->w_layout) flag |= LV_TEXT_FLAG_FIT;

    lv_txt_get_size(&size, dclock->text, font, letter_space, line_space, max_w, flag);

    lv_obj_refresh_self_size(obj);

    lv_obj_invalidate(obj);

}

#endif
