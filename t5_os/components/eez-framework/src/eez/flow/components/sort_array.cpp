/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <eez/conf-internal.h>

#include <string.h>
#include <stdlib.h>

#include <eez/core/util.h>
#include <eez/core/debug.h>
#include <eez/core/utf8.h>

#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/expression.h>
#include <eez/flow/debugger.h>

#include <eez/flow/components/sort_array.h>

namespace eez {
namespace flow {

SortArrayActionComponent *g_sortArrayActionComponent;

int elementCompare(const void *a, const void *b) {
    auto aValue = *(const Value *)a;
    auto bValue = *(const Value *)b;

    if (g_sortArrayActionComponent->arrayType != -1) {
        if (!aValue.isArray()) {
            return 0;
        }
        auto aArray = aValue.getArray();
        if ((uint32_t)g_sortArrayActionComponent->structFieldIndex >= aArray->arraySize) {
            return 0;
        }
        aValue = aArray->values[g_sortArrayActionComponent->structFieldIndex];

        if (!bValue.isArray()) {
            return 0;
        }
        auto bArray = bValue.getArray();
        if ((uint32_t)g_sortArrayActionComponent->structFieldIndex >= bArray->arraySize) {
            return 0;
        }
        bValue = bArray->values[g_sortArrayActionComponent->structFieldIndex];
    }

    int result;

    if (aValue.isString() && bValue.isString()) {
        if (g_sortArrayActionComponent->flags & SORT_ARRAY_FLAG_IGNORE_CASE) {
            result = utf8casecmp(aValue.getString(), bValue.getString());
        } else {
            result = utf8cmp(aValue.getString(), bValue.getString());
        }
    } else {
        int err;
        float aDouble = aValue.toDouble(&err);
        if (err) {
            return 0;
        }
        float bDouble = bValue.toDouble(&err);
        if (err) {
            return 0;
        }

        auto diff = aDouble - bDouble;
        result = diff < 0 ? -1 : diff > 0 ? 1 : 0;
    }

    if (!(g_sortArrayActionComponent->flags & SORT_ARRAY_FLAG_ASCENDING)) {
        result = -result;
    }

    return result;
}

void sortArray(SortArrayActionComponent *component, ArrayValue *array) {
    g_sortArrayActionComponent = component;
    qsort(&array->values[0], array->arraySize, sizeof(Value), elementCompare);

}

void executeSortArrayComponent(FlowState *flowState, unsigned componentIndex) {
    auto component = (SortArrayActionComponent *)flowState->flow->components[componentIndex];

    Value srcArrayValue;
    if (!evalProperty(flowState, componentIndex, defs_v3::SORT_ARRAY_ACTION_COMPONENT_PROPERTY_ARRAY, srcArrayValue, "Failed to evaluate Array in SortArray\n")) {
        return;
    }

    if (!srcArrayValue.isArray()) {
        throwError(flowState, componentIndex, "SortArray: not an array\n");
        return;
    }

    auto arrayValue = srcArrayValue.clone();
    auto array = arrayValue.getArray();

    if (component->arrayType != -1) {
        if (array->arrayType != (uint32_t)component->arrayType) {
            throwError(flowState, componentIndex, "SortArray: invalid array type\n");
            return;
        }

        if (component->structFieldIndex < 0) {
            throwError(flowState, componentIndex, "SortArray: invalid struct field index\n");
        }
    } else {
        if (array->arrayType != defs_v3::ARRAY_TYPE_INTEGER && array->arrayType != defs_v3::ARRAY_TYPE_FLOAT && array->arrayType != defs_v3::ARRAY_TYPE_DOUBLE && array->arrayType != defs_v3::ARRAY_TYPE_STRING) {
            throwError(flowState, componentIndex, "SortArray: array type is neither array:integer or array:float or array:double or array:string\n");
            return;
        }
    }

    sortArray(component, array);

	propagateValue(flowState, componentIndex, component->outputs.count - 1, arrayValue);
}

} // namespace flow
} // namespace eez
