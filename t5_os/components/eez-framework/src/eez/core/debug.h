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

#ifdef DEBUG

#include <stdlib.h>
#include <stdint.h>

namespace eez {
namespace debug {

void pushDebugTraceHook(const char *message, size_t messageLength);
void pushInfoTraceHook(const char *message, size_t messageLength);
void pushErrorTraceHook(const char *message, size_t messageLength);

enum TraceType {
    TRACE_TYPE_DEBUG,
    TRACE_TYPE_INFO,
    TRACE_TYPE_ERROR
};

void Trace(TraceType traceType, const char *format, ...);

} // namespace debug
} // namespace eez

#define InfoTrace(...) ::eez::debug::Trace(::eez::debug::TRACE_TYPE_INFO, __VA_ARGS__)
#define ErrorTrace(...) ::eez::debug::Trace(::eez::debug::TRACE_TYPE_ERROR, __VA_ARGS__)
#define DebugTrace(...) ::eez::debug::Trace(::eez::debug::TRACE_TYPE_DEBUG, __VA_ARGS__)

#else // NO DEBUG

#define InfoTrace(...) (void)0
#define ErrorTrace(...) (void)0
#define DebugTrace(...) (void)0

#endif
