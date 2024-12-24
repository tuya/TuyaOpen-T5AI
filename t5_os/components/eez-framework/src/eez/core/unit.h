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

#define INFINITY_SYMBOL "\x91"
#define DEGREE_SYMBOL "\x8a"

namespace eez {

// order of units should not be changed since it is used in DLOG files
enum Unit {
    UNIT_UNKNOWN = 255,
    UNIT_NONE = 0,
    UNIT_VOLT,
    UNIT_MILLI_VOLT,
    UNIT_AMPER,
    UNIT_MILLI_AMPER,
    UNIT_MICRO_AMPER,
    UNIT_WATT,
    UNIT_MILLI_WATT,
    UNIT_SECOND,
    UNIT_MILLI_SECOND,
    UNIT_CELSIUS,
    UNIT_RPM,
    UNIT_OHM,
    UNIT_KOHM,
    UNIT_MOHM,
    UNIT_PERCENT,
    UNIT_HERTZ,
    UNIT_MILLI_HERTZ,
    UNIT_KHERTZ,
    UNIT_MHERTZ,
    UNIT_JOULE,
    UNIT_FARAD,
    UNIT_MILLI_FARAD,
    UNIT_MICRO_FARAD,
    UNIT_NANO_FARAD,
    UNIT_PICO_FARAD,
    UNIT_MINUTE,
    UNIT_VOLT_AMPERE,
    UNIT_VOLT_AMPERE_REACTIVE,
	UNIT_DEGREE,
	UNIT_VOLT_PP,
	UNIT_MILLI_VOLT_PP,
	UNIT_AMPER_PP,
	UNIT_MILLI_AMPER_PP,
	UNIT_MICRO_AMPER_PP,
};

extern const char *g_unitNames[];

inline const char *getUnitName(Unit unit) {
	if (unit == UNIT_UNKNOWN) {
		return "";
	}
    return g_unitNames[unit];
}

Unit getUnitFromName(const char *unitName);

#if OPTION_SCPI
int getScpiUnit(Unit unit);
#endif

// for UNIT_MILLI_VOLT returns UNIT_VOLT, etc...
Unit getBaseUnit(Unit unit);

// returns 1.0 form UNIT_VOLT, returns 1E-3 for UNIT_MILLI_VOLT, 1E-6 for UNIT_MICRO_AMPER, 1E3 for UNIT_KOHM, etc
float getUnitFactor(Unit unit);

// if value is 0.01 and unit is UNIT_VOLT returns UNIT_MILLI_VOLT, etc
Unit findDerivedUnit(float value, Unit unit);

Unit getSmallerUnit(Unit unit, float min, float precision);
Unit getBiggestUnit(Unit unit, float max);
Unit getSmallestUnit(Unit unit, float min, float precision);

} // namespace eez
