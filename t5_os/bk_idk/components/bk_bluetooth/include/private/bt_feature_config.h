#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


struct bt_feat_funcs_t
{
    bool (*_is_gatt_discovery_auto)(void);
};
