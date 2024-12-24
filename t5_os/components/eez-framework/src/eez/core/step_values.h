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
#include <eez/core/value.h>

namespace eez {

enum EncoderMode {
	ENCODER_MODE_MIN,
	ENCODER_MODE_AUTO = ENCODER_MODE_MIN,
	ENCODER_MODE_STEP1,
	ENCODER_MODE_STEP2,
	ENCODER_MODE_STEP3,
	ENCODER_MODE_STEP4,
	ENCODER_MODE_STEP5,
	ENCODER_MODE_MAX = ENCODER_MODE_STEP5
};

struct StepValues {
	int count;
	const float *values;
	Unit unit;
	struct {
		bool accelerationEnabled;
		float range;
		float step;
		EncoderMode mode;
	} encoderSettings;
};

inline StepValues *getStepValues(const Value &value) { return (StepValues *)value.pVoidValue; }

} // namespace eez
