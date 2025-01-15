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

#ifdef DEBUG

#include <cstdio>
#include <stdarg.h>
#include <string.h>

#include <eez/core/debug.h>

namespace eez {
namespace debug {

void Trace(TraceType traceType, const char *format, ...) {
    va_list args;
    va_start(args, format);

    static const size_t BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE + 1];

	vsnprintf(buffer, BUFFER_SIZE, format, args);
	buffer[BUFFER_SIZE] = 0;

    va_end(args);

    if (traceType == TRACE_TYPE_DEBUG) {
        pushDebugTraceHook(buffer, strlen(buffer));
    } else if (traceType == TRACE_TYPE_INFO) {
        pushInfoTraceHook(buffer, strlen(buffer));
    } else {
        pushErrorTraceHook(buffer, strlen(buffer));
    }
}

} // namespace debug
} // namespace eez

extern "C" void debug_trace(const char *str, size_t len) {
    eez::debug::pushDebugTraceHook(str, len);
}

#endif // DEBUG
