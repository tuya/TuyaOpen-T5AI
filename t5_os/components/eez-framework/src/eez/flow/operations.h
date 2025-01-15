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

#include <eez/flow/expression.h>

namespace eez {
namespace flow {

typedef void (*EvalOperation)(EvalStack &);

extern EvalOperation g_evalOperations[];

Value op_add(const Value& a1, const Value& b1);
Value op_sub(const Value& a1, const Value& b1);
Value op_mul(const Value& a1, const Value& b1);
Value op_div(const Value& a1, const Value& b1);
Value op_mod(const Value& a1, const Value& b1);

Value op_left_shift(const Value& a1, const Value& b1);
Value op_right_shift(const Value& a1, const Value& b1);
Value op_binary_and(const Value& a1, const Value& b1);
Value op_binary_or(const Value& a1, const Value& b1);
Value op_binary_xor(const Value& a1, const Value& b1);

Value op_eq(const Value& a1, const Value& b1);
Value op_neq(const Value& a1, const Value& b1);
Value op_less(const Value& a1, const Value& b1);
Value op_great(const Value& a1, const Value& b1);
Value op_less_eq(const Value& a1, const Value& b1);
Value op_great_eq(const Value& a1, const Value& b1);

} // flow
} // eez
