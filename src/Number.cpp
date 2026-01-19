/****
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

namespace ConfigDB
{
/**
 * @brief Return the adjusted exponent after shifting out the coefficient’s
 * rightmost digits until only the lead digit remains.
 *
 * Number('321e+5').adjusted() returns 7.
 * Used for determining the position of the most significant digit
 * with respect to the decimal point.
 */
int adjustExponent(number_t num)
{
	int exp = num.exponent;
	unsigned absMantissa = abs(num.mantissa);
	unsigned m{10};
	while(m < absMantissa) {
		++exp;
		m *= 10;
	}
	return exp;
}

int number_t::compare(number_t num1, number_t num2)
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
	auto exp1 = adjustExponent(num1);
	auto exp2 = adjustExponent(num2);
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

bool number_t::parse(const char* value, unsigned length, number_t& number)
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
				return false;
			}
			mantissa = c - '0';
			state = State::mant;
			break;

		case State::mant:
			if(c == '.') {
				state = State::frac;
				break;
			}
			if(c == 'e' || c == 'E') {
				state = State::exp;
				break;
			}
			if(!isdigit(c)) {
				return false;
			}
			if(mantissa < 0xfffffffful / 10) {
				mantissa = (mantissa * 10) + c - '0';
			} else {
				++shift;
			}
			break;

		case State::frac:
			if(c == 'e' || c == 'E') {
				state = State::exp;
				break;
			}
			if(!isdigit(c)) {
				return false;
			}
			if(mantissa <= 0xfffffffful / 10) {
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
				return false;
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

	number = normalise(mantissa, shift, isNeg);
	return true;
}

const char* number_t::format(char* buf, number_t number)
{
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

double number_t::asFloat(number_t number)
{
	double value = number.mantissa;
	for(int i = 0; i < number.exponent; ++i) {
		value *= 10;
	}
	for(int i = 0; i > number.exponent; --i) {
		value /= 10;
	}
	return value;
}

int64_t number_t::asInt64(number_t number)
{
	int64_t value = number.mantissa;
	int exponent = number.exponent;
	while(value && exponent < -1) {
		value /= 10;
		++exponent;
	}
	while(value && exponent < 0) {
		auto newValue = (value + 5) / 10;
		if(newValue < value) {
			++exponent;
		}
	}
	const auto maxval = std::numeric_limits<int64_t>::max();
	while(exponent > 0) {
		if(abs(value) > maxval / 10) {
			return (value < 0) ? -maxval : maxval;
		}
		value *= 10;
		--exponent;
	}
	return value;
}

} // namespace ConfigDB
