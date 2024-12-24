/**
 * @file lv_dclock.h
 *
 */

#ifndef LV_DCLOCK_H
#define LV_DCLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../lv_conf_internal.h"

#if LV_USE_DCLOCK != 0

#include <stdarg.h>
#include <stdlib.h>
#include "../../../core/lv_obj.h"
#include "../../../font/lv_font.h"
#include "../../../font/lv_symbol_def.h"
#include "../../../misc/lv_txt.h"
#include "../../../draw/lv_draw.h"

/*********************
 *      DEFINES
 *********************/
#define LV_DCLOCK_WAIT_CHAR_COUNT        3
#define LV_DCLOCK_DOT_NUM 3
#define LV_DCLOCK_POS_LAST 0xFFFF
#define LV_DCLOCK_TEXT_SELECTION_OFF LV_DRAW_LABEL_NO_TXT_SEL


LV_EXPORT_CONST_INT(LV_DCLOCK_DOT_NUM);
LV_EXPORT_CONST_INT(LV_DCLOCK_POS_LAST);
LV_EXPORT_CONST_INT(LV_DCLOCK_TEXT_SELECTION_OFF);

/**********************
 *      TYPEDEFS
 **********************/


typedef struct {
    lv_obj_t obj;
    char * text;

#if LV_DCLOCK_TEXT_SELECTION
    uint32_t sel_start;
    uint32_t sel_end;
#endif

    lv_point_t offset; /*Text draw position offset*/
    uint8_t static_txt : 1;             /*Flag to indicate the text is static*/
    uint8_t recolor : 1;                /*Enable in-line letter re-coloring*/
    uint8_t expand : 1;                 /*Ignore real width (used by the library with LV_LABEL_LONG_SCROLL)*/
} lv_dclock_t;


extern const lv_obj_class_t lv_dclock_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a dclock object
 * @param parent    pointer to an object, it will be the parent of the new label.
         input_time  Type the time as the following:
 * @return          pointer to the created button
 * Example:  lv_obj_t * dclock = lv_dclock_create(lv_obj_t * parent,"11:59:55 AM", LV_DCLOCK_SHOW_SECONDS_TRUE,LV_DCLOCK_SHOW_MERIDIEM_TRUE);
 */

lv_obj_t * lv_dclock_create(lv_obj_t * parent, char * input_time);

/*=====================
 * Setter functions
 *====================*/

void lv_dclock_set_text_fmt(lv_obj_t * obj, const char * fmt, ...) LV_FORMAT_ATTRIBUTE(2, 3);

void lv_dclock_set_text(lv_obj_t * obj, const char * text);

/**
 * @brief           Calculate the digital clock by the 12-hours mode
 * @param obj       pointer to the values for hour/minute/seconds and the bool value to select AM/PM
 * @return          selection end index. The function of lv_dclock_12_timer_cb will call it.
 */
void clock_count_12(int * hour, int * minute, int * seconds, char * meridiem);


/**
 * @brief           Calculate the digital clock by the 24-hours mode
 * @param obj       pointer to the values for hour/minute/seconds and the bool value to select AM/PM
 * @return          selection end index. The function of lv_dclock_24_timer_cb will call it.
 */
void clock_count_24(int * hour, int * minute, int * seconds);

/*=====================
 * Getter functions
 *====================*/

/**
 * @brief Get the selection start index.
 * @param obj       pointer to a dclock object.
 * @return          selection start index. `LV_LABEL_TEXT_SELECTION_OFF` if nothing is selected.
 */
uint32_t lv_dclock_get_text_selection_start(const lv_obj_t * obj);

/**
 * @brief Get the selection end index.
 * @param obj       pointer to a dclock object.
 * @return          selection end index. `LV_LABEL_TXT_SEL_OFF` if nothing is selected.
 */
uint32_t lv_dclock_get_text_selection_end(const lv_obj_t * obj);


/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DCLOCK*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DCLOCK_H*/
