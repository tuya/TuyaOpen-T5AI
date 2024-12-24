/**
 * @file lv_textprogress.h
 *
 */

#ifndef LV_TEXTPROGRESS_H
#define LV_TEXTPROGRESS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"

#if LV_USE_TEXTPROGRESS
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    lv_label_t text;
    uint32_t range_max;
    uint32_t range_min;
    uint32_t value;
    uint32_t range_steps;
    uint32_t range_steps_min;
    uint16_t decimals;
} lv_textprogress_t;

extern const lv_obj_class_t lv_textprogress_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a textprogress object
 * @param parent    pointer to an object, it will be the parent of textprogress
 * @return          pointer to the created textprogress object
 */
lv_obj_t * lv_textprogress_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set range value of a textprogress object
 * @param obj       pointer to an object, it is a textprogress to set range value
 * @param min       the minimum value of range
 * @param max       the maximum value of range
 * @param steps     the number of steps from min to max range
 * @param steps_min the minimum steps of textprogress
 * @return          pointer to chenged textprogress
 */
void lv_textprogress_set_range_value(lv_obj_t * obj, uint32_t min, uint32_t max, uint32_t steps, uint32_t steps_min);

/**
 * Set decimal place of a textprogress object
 * @param obj       pointer to an object, it is a textprogress to set decimal place
 * @param decimal   the decimal place of the textprogress
 * @return          pointer to chenged textprogress
 */
void lv_textprogress_set_decimal(lv_obj_t * obj, uint32_t decimal);

/**
 * Set current value of a textprogress object
 * @param obj       pointer to an object, it is a textprogress to set current value
 * @param value     the value want to set for the textprogress
 * @return          pointer to chenged textprogress
 */
void lv_textprogress_set_value(lv_obj_t * obj, uint32_t value);


/*=====================
 * Getter functions
 *====================*/

/**
 * Get each step's progress of a textprogress object
 * @param radiobtn  pointer to a textprogress object
 * @param range     the range of textprogress object
 * @return          the progresss number of textprogress
 */
uint32_t lv_textprogress_get_progress(lv_obj_t * obj, uint32_t range);


#endif /*LV_USE_TEXTPROGRESS*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_TEXTPROGRESS_H*/