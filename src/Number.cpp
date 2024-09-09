/**
 * ConfigDB/Number.cpp
 *
 * Copyright 2024 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the ConfigDB Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#include "include/ConfigDB/Number.h"
#include <stringconversion.h>
#include <debug_progmem.h>
#include <cmath>

// Defined in newlib but not ANSI or GNU standard
extern "C" char* ecvtbuf(double invalue, int ndigit, int* decpt, int* sign, char* fcvt_buf);

namespace ConfigDB
{
int number_t::adjustedExponent() const
{
	int exp = exponent;
	unsigned absMantissa = abs(mantissa);
	unsigned m{10};
	while(m < absMantissa) {
		++exp;
		m *= 10;
	}
	return exp;
}

int number_t::compare(const number_t& num1, const number_t& num2)
{
	/*
	 * Thanks to cpython `Decimal._cmp` for this implementation.
	 *
	 * https://github.com/python/cpython/blob/033510e11dff742d9626b9fd895925ac77f566f1/Lib/_pydecimal.py#L730
	 */

	if(num1 == num2) {
		return 0;
	}

	// Check for zeroes
	if(num1.mantissa == 0) {
		if(num2.mantissa == 0) {
			return 0;
		}
		return num2.sign() ? 1 : -1;
	}
	if(num2.mantissa == 0) {
		return num1.sign() ? -1 : 1;
	}

	// Compare signs
	if(num1.sign() < num2.sign()) {
		return 1;
	}
	if(num1.sign() > num2.sign()) {
		return -1;
	}

	// OK, both numbers have same sign
	auto exp1 = num1.adjustedExponent();
	auto exp2 = num2.adjustedExponent();
	if(exp1 < exp2) {
		return num1.sign() ? 1 : -1;
	}
	if(exp1 > exp2) {
		return num1.sign() ? -1 : 1;
	}

	// Compare mantissa. Watch for e.g. 10e9 vs 12e9
	int m1 = num1.mantissa;
	int m2 = num2.mantissa;
	int diff = num1.exponent - num2.exponent;
	while(diff > 0) {
		m1 *= 10;
		--diff;
	}
	while(diff < 0) {
		m2 *= 10;
		++diff;
	}
	if(m1 < m2) {
		return -1;
	}
	if(m1 > m2) {
		return 1;
	}
	return 0;
}

number_t Number::parse(int64_t value)
{
	int exponent = 0;
	bool isNeg{false};
	if(value < 0) {
		isNeg = true;
		value = -value;
	}
	while(value > 0x7fffffff) {
		value /= 10;
		++exponent;
	}
	return normalise(value, exponent, isNeg);
}

number_t Number::parse(double value)
{
	if(value == 0) {
		return {};
	}

	if(std::isnan(value)) {
		return invalid;
	}

	if(std::isinf(value)) {
		return overflow;
	}

	unsigned mantissa{0};
	int sign{0};
	int exponent{0};

#ifdef ARCH_HOST
	char buf[number_t::minBufferSize];
	sprintf(buf, "%.6e", value);

	auto exp = strchr(buf, 'e');
	*exp++ = '\0';
	exponent = atoi(exp) + 1;
	int mlen = strlen(buf);
	while(buf[mlen - 1] == '0') {
		--mlen;
	}
	buf[mlen] = '\0';
	auto dp = strchr(buf, '.');
	while(*dp) {
		dp[0] = dp[1];
		++dp;
		--exponent;
	}
	int m = atoi(buf);
	if(m < 0) {
		sign = 1;
		mantissa = -m;
	} else {
		mantissa = m;
	}
#else
	int decpt;
	char buf[number_t::maxSignificantDigits + 1];
	ecvtbuf(value, number_t::maxSignificantDigits, &decpt, &sign, buf);
	mantissa = atoi(buf);
	exponent = decpt - number_t::maxSignificantDigits;
#endif

	return normalise(mantissa, exponent, sign);
}

number_t Number::parse(const char* value, unsigned length)
{
	enum class State {
		sign,
		mant,
		frac,
		exp,
		expval,
	};
	State state{};

	bool isNeg{false};
	unsigned mantissa{0};
	int shift{0};
	unsigned exponent{0};
	bool expIsNeg{false};
	for(auto valptr = value; length--;) {
		char c = *valptr++;
		switch(state) {
		case State::sign:
			if(c == '+') {
				break;
			}
			if(c == '-') {
				isNeg = true;
				break;
			}
			if(c == '.') {
				state = State::frac;
				break;
			}
			if(!isdigit(c)) {
				return invalid;
			}
			mantissa = c - '0';
			state = State::mant;
			break;

		case State::mant:
			if(c == '.') {
				state = State::frac;
				break;
			}
			if(c == 'e') {
				state = State::exp;
				break;
			}
			if(!isdigit(c)) {
				return invalid;
			}
			if(mantissa > 0x7fffffff / 10) {
				return overflow;
			}
			mantissa = (mantissa * 10) + c - '0';
			break;

		case State::frac:
			if(c == 'e') {
				state = State::exp;
				break;
			}
			if(!isdigit(c)) {
				return invalid;
			}
			if(mantissa <= 0x7fffffff / 10) {
				mantissa = (mantissa * 10) + c - '0';
				--shift;
			}
			break;

		case State::exp:
			if(c == '+') {
				state = State::expval;
				break;
			}
			if(c == '-') {
				expIsNeg = true;
				state = State::expval;
				break;
			}
			state = State::expval;
			[[fallthrough]];

		case State::expval:
			if(!isdigit(c)) {
				return invalid;
			}
			exponent = (exponent * 10) + c - '0';
			break;
		}
	}

	// debug_i("value %s, mant %u, exp %d, shift %d", value, mantissa, exponent, shift);

	if(expIsNeg) {
		shift -= exponent;
	} else {
		shift += exponent;
	}

	return normalise(mantissa, shift, isNeg);
}

number_t Number::normalise(unsigned mantissa, int exponent, bool isNeg)
{
	// Round mantissa down if it exceeds maximum precision
	// mlim is used to detect when mantissa gains a digit when rounded up (e.g. 995 to 1000)
	// TODO: There *is* a much more efficient way to do this. Find it.
	if(mantissa > number_t::maxMantissa) {
		uint64_t mlim = 1;
		while(mlim < mantissa) {
			mlim *= 10;
		}
		do {
			mantissa = (mantissa + 5) / 10;
			if(mantissa < mlim) {
				++exponent;
				mlim /= 10;
			}
		} while(mantissa > number_t::maxMantissa);
	}

	// Drop any trailing 0's from mantissa
	while(mantissa >= 10 && mantissa % 10 == 0) {
		mantissa /= 10;
		++exponent;
	}

	// Adjust exponent to keep it in range (without losing precision)
	while(exponent > int(number_t::maxExponent)) {
		mantissa *= 10;
		--exponent;
	}

	// Check for overflow conditions
	if(mantissa > number_t::maxMantissa) {
		return overflow;
	}
	if(abs(exponent) > number_t::maxExponent) {
		return overflow;
	}

	// Success
	number_t num;
	num.mantissa = isNeg ? -mantissa : mantissa;
	num.exponent = exponent;
	return num;
}

String Number::toString() const
{
	char buf[number_t::minBufferSize];
	return format(buf, number);
}

const char* Number::format(char* buf, number_t number)
{
	if(number == overflow) {
		strcpy(buf, "OVF");
		return buf;
	}

	if(number == invalid) {
		strcpy(buf, "NaN");
		return buf;
	}

	int mantissa = number.mantissa;
	int exponent = number.exponent;
	while(mantissa % 10 == 0 && mantissa >= 10) {
		mantissa /= 10;
		++exponent;
	}

	// Output mantissa
	auto numptr = buf;
	if(mantissa < 0) {
		*numptr++ = '-';
	}
	ultoa(abs(mantissa), numptr, 10);
	int mlen = strlen(numptr);

	// Case 1: exponent == 0 (e.g. 0, 1, 2, 3)
	if(exponent == 0) {
		return buf;
	}

	// Determine what exponent would be for 'e' form
	int exp = exponent + mlen - 1;
	if(exp <= -4 || exp >= 6) {
		if(numptr[1]) {
			memmove(numptr + 2, numptr + 1, mlen - 1);
			numptr[1] = '.';
			++mlen;
		}
		numptr += mlen;
		*numptr++ = 'e';
		itoa(exp, numptr, 10);
		return buf;
	}

	// Case 1: exponent > 0, append 0's
	if(exponent > 0) {
		numptr += mlen;
		memset(numptr, '0', exponent);
		numptr[exponent] = '\0';
		return buf;
	}

	// Case 2: exponent < 0, insert 0's
	int insertLen = -(mlen + exponent);
	if(insertLen >= 0) {
		memmove(numptr + insertLen + 2, numptr, mlen + 1);
		*numptr++ = '0';
		*numptr++ = '.';
		memset(numptr, '0', insertLen);
		return buf;
	}

	// Case 3: insert dp
	numptr += mlen + exponent;
	memmove(numptr + 1, numptr, 1 - exponent);
	*numptr = '.';
	return buf;
}

size_t Number::printTo(Print& p) const
{
	char buf[number_t::minBufferSize];
	auto str = format(buf, number);
	return p.print(str);
}

double Number::asFloat() const
{
	if(number == invalid) {
		return NAN;
	}
	if(number == overflow) {
		return HUGE_VALF;
	}

	char buf[number_t::minBufferSize];
	auto str = format(buf, number);
	return strtod(str, nullptr);
}

int64_t Number::asInt64() const
{
	if(number == invalid) {
		return 0;
	}
	if(number == overflow) {
		return 0;
	}
	int64_t value = number.mantissa;
	int exponent = number.exponent;
	while(value && exponent < 0) {
		value = (value + 5) / 10;
		++exponent;
	}
	while(exponent > 0) {
		if(value > std::numeric_limits<int64_t>::max() / 10) {
			return 0;
		}
		value *= 10;
		--exponent;
	}
	return value;
}

} // namespace ConfigDB
