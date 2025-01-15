#include "bt_feature_config.h"

#include "os/os.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//only valid when use single ble host.
static bool bt_feat_is_gatt_discovery_auto(void)
{
#if CONFIG_BLUETOOTH_BLE_DISCOVER_AUTO
    return CONFIG_BLUETOOTH_BLE_DISCOVER_AUTO;
#else
    return false;
#endif
}

static const struct bt_feat_funcs_t bt_feat_funcs =
{
    ._is_gatt_discovery_auto = bt_feat_is_gatt_discovery_auto,
};


int bk_bt_feature_init(void)
{
    int ret = BK_OK;
    extern int bt_feature_adapter_init(void *osi_funcs);

    if (bt_feature_adapter_init((void *)&bt_feat_funcs) != 0)
    {
        return BK_FAIL;
    }

    return ret;
}
