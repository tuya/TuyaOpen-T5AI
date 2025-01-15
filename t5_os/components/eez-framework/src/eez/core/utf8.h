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

#ifndef UTF8_SUPPORT
#define UTF8_SUPPORT 1
#endif

#if UTF8_SUPPORT

#include <eez/libs/utf8.h>

#else

#include <string.h>

typedef char utf8_int8_t;
typedef int32_t utf8_int32_t;

inline const utf8_int8_t* utf8codepoint(const utf8_int8_t *str, utf8_int32_t *out_codepoint) {
    *out_codepoint = *((uint8_t *)str);
    return str + 1;
}

inline utf8_int8_t *utf8catcodepoint(utf8_int8_t *str, utf8_int32_t chr, size_t n) {
    if (n < 1) return nullptr;
    str[0] = (char)chr;
    return str + 1;
}

#define utf8len strlen
#define utf8cmp strcmp

#ifdef _MSC_VER
#define utf8casecmp _stricmp
#else
#define utf8casecmp strcasecmp
#endif

#endif
