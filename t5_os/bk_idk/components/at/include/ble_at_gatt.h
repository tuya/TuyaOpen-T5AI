#pragma once
#include "os/os.h"

#define bt_at_logi(format, ...) do{BK_LOGI("ble_at", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define bt_at_logw(format, ...) do{BK_LOGW("ble_at", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define bt_at_loge(format, ...) do{BK_LOGE("ble_at", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

enum
{
    DB_TYPE_TIMER_SEND = 0,
    DB_TYPE_PERFORMANCE
};

typedef struct
{
    bk_gatt_if_t gattc_if;
    bk_gatt_if_t gatts_if;
    uint16_t conn_handle;
}ble_at_gatt_ctx_handle_t;


ble_at_gatt_ctx_handle_t *ble_at_get_gatt_ctx_handle(void);


int32_t ble_at_gatts_reg(void);
int32_t ble_at_gattc_reg(void);
int32_t ble_at_reg_gatts_db(uint8_t db_type);

