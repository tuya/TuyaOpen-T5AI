/**
 * @file lv_textprogress.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_textprogress.h"

#if LV_USE_TEXTPROGRESS != 0

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_textprogress_class

#define MULDIV(a,b,c) (a*b)/c

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_textprogress_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_textprogress_class = {
    .constructor_cb = lv_textprogress_constructor,
    .instance_size = sizeof(lv_textprogress_t),
    .base_class = &lv_label_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_textprogress_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_textprogress_set_range_value(lv_obj_t * obj, uint32_t min, uint32_t max, uint32_t steps, uint32_t steps_min)
{
    lv_textprogress_t * tp = (lv_textprogress_t *)obj;

    tp->range_min = min;
    tp->range_max = max;
    tp->range_steps_min = steps_min;
    if(steps == 0) {
        tp->range_steps = max - min;
    }
    else {
        tp->range_steps = steps;
    }
}

void lv_textprogress_set_decimal(lv_obj_t * obj, uint32_t decimal)
{
    lv_textprogress_t * tp = (lv_textprogress_t *)obj;

    tp->decimals = (((2) < (decimal)) ? (2) : (decimal));
}

void lv_textprogress_set_value(lv_obj_t * obj, uint32_t value)
{
    lv_textprogress_t * tp = (lv_textprogress_t *)obj;

    value = (((value) > (tp->range_min)) ? (value) : (tp->range_min));
    value = (((value) < (tp->range_max)) ? (value) : (tp->range_max));
    tp->value = value;
    int range[3] = {1, 10, 100};
    uint32_t progress;
    char textbuffer[8];
    progress = lv_textprogress_get_progress(obj, 100 * range[tp->decimals]);
    if(tp->decimals > 0) {
        lv_snprintf(textbuffer, sizeof(textbuffer), "%d.%0*d%%", (progress / range[tp->decimals]), tp->decimals,
                    (progress % range[tp->decimals]));
    }
    else {
        lv_snprintf(textbuffer, sizeof(textbuffer), "%d%%", progress);
    }
    lv_label_set_text(obj, textbuffer);
}
/*=====================
 * Getter functions
 *====================*/

uint32_t lv_textprogress_get_progress(lv_obj_t * obj, uint32_t range)
{
    lv_textprogress_t * tp = (lv_textprogress_t *)obj;

    int32_t step, prog;
    step = tp->range_steps_min + MULDIV(((tp->value) - (tp->range_min)), ((tp->range_steps) - (tp->range_steps_min)),
                                        ((tp->range_max) - (tp->range_min)));
    prog = MULDIV(step, range, tp->range_steps);
    return prog;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_textprogress_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_textprogress_t * tp = (lv_textprogress_t *)obj;
    tp->range_min = 0;
    tp->range_max = 100;
    tp->range_steps = 80;
    tp->range_steps_min = 0;
    tp->value = 0;
    tp->decimals = 2;

    lv_label_set_text(obj, "0.00%");

    LV_TRACE_OBJ_CREATE("finished");
}


#endif /*LV_USE_TEXTPROGRESS*/