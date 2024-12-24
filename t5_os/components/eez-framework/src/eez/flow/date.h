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

#include <stdint.h>

namespace eez {
namespace flow {
namespace date {

enum DstRule { DST_RULE_OFF, DST_RULE_EUROPE, DST_RULE_USA, DST_RULE_AUSTRALIA };
enum Format { FORMAT_DMY_24, FORMAT_MDY_24, FORMAT_DMY_12, FORMAT_MDY_12 };

typedef uint64_t Date;

extern Format g_localeFormat;
extern int g_timeZone;
extern DstRule g_dstRule;

Date now();

void toString(Date time, char *str, uint32_t strLen);
void toLocaleString(Date time, char *str, uint32_t strLen);
Date fromString(const char *str);

Date makeDate(int year, int month, int day, int hours, int minutes, int seconds, int milliseconds);

void breakDate(Date time, int &year, int &month, int &day, int &hours, int &minutes, int &seconds, int &milliseconds);

int getYear(Date time);
int getMonth(Date time);
int getDay(Date time);
int getHours(Date time);
int getMinutes(Date time);
int getSeconds(Date time);
int getMilliseconds(Date time);

Date utcToLocal(Date utc);
Date localToUtc(Date local);

} // namespace date
} // namespace flow
} // namespace eez
