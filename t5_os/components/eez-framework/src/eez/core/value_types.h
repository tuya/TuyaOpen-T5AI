/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <eez/core/unit.h>

#define VALUE_TYPES \
    VALUE_TYPE(UNDEFINED)                          /*  0 */ \
    VALUE_TYPE(NULL)                               /*  1 */ \
    VALUE_TYPE(BOOLEAN)                            /*  2 */ \
    VALUE_TYPE(INT8)                               /*  3 */ \
    VALUE_TYPE(UINT8)                              /*  4 */ \
    VALUE_TYPE(INT16)                              /*  5 */ \
    VALUE_TYPE(UINT16)                             /*  6 */ \
    VALUE_TYPE(INT32)                              /*  7 */ \
    VALUE_TYPE(UINT32)                             /*  8 */ \
    VALUE_TYPE(INT64)                              /*  9 */ \
    VALUE_TYPE(UINT64)                             /* 10 */ \
    VALUE_TYPE(FLOAT)                              /* 11 */ \
	VALUE_TYPE(DOUBLE)                             /* 12 */ \
    VALUE_TYPE(STRING)                             /* 13 */ \
    VALUE_TYPE(STRING_ASSET)                       /* 14 */ \
    VALUE_TYPE(ARRAY)                              /* 15 */ \
    VALUE_TYPE(ARRAY_ASSET)                        /* 16 */ \
	VALUE_TYPE(STRING_REF)                         /* 17 */ \
    VALUE_TYPE(ARRAY_REF)                          /* 18 */ \
    VALUE_TYPE(BLOB_REF)                           /* 19 */ \
    VALUE_TYPE(STREAM)                             /* 20 */ \
    VALUE_TYPE(DATE)                               /* 21 */ \
	VALUE_TYPE(VERSIONED_STRING)                   /* 22 */ \
	VALUE_TYPE(VALUE_PTR)                          /* 23 */ \
    VALUE_TYPE(ARRAY_ELEMENT_VALUE)                /* 24 */ \
    VALUE_TYPE(FLOW_OUTPUT)                        /* 25 */ \
    VALUE_TYPE(NATIVE_VARIABLE)                    /* 26 */ \
    VALUE_TYPE(ERROR)                              /* 27 */ \
    VALUE_TYPE(RANGE)                              /* 28 */ \
    VALUE_TYPE(POINTER)                            /* 29 */ \
    VALUE_TYPE(ENUM)                               /* 30 */ \
    VALUE_TYPE(IP_ADDRESS)                         /* 31 */ \
    VALUE_TYPE(TIME_ZONE)                          /* 32 */ \
    VALUE_TYPE(YT_DATA_GET_VALUE_FUNCTION_POINTER) /* 33 */ \
    CUSTOM_VALUE_TYPES

namespace eez {

#define VALUE_TYPE(NAME) VALUE_TYPE_##NAME,
enum ValueType {
	VALUE_TYPES
};
#undef VALUE_TYPE

}
