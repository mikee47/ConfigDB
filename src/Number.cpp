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

namespace ConfigDB
{
number_t Number::parse(double value)
{
	if(value == 0) {
		return {};
	}

#ifdef ARCH_HOST
	char* buf;
	asprintf(&buf, "%.6e", value);

	auto exp = strchr(buf, 'e');
	*exp++ = '\0';
	int exponent = atoi(exp) + 1;
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
	int mantissa = atoi(buf);
	free(buf);
	return {mantissa, exponent};
#else
	int decpt;
	int sign;
	char* rve;
	auto buf = _dtoa_r(_REENT, value, 2, 7, &decpt, &sign, &rve);
	auto len = strlen(buf);
	char c{};
	if(len > 7) {
		c = buf[7];
		buf[7] = '\0';
		len = 7;
	}
	int mantissa = atoi(buf);
	if(c > '5') {
		++mantissa;
	}
	if(sign) {
		mantissa = -mantissa;
	}
	return {mantissa, decpt - len};
#endif
}

number_t Number::parse(const char* value, unsigned length)
{
	enum class State {
		sign,
		mant,
		frac,
		exp,
		rounded,
	};
	State state{};

	bool isNeg{false};
	unsigned mantissa{0};
	int shift{0};
	unsigned exponent{0};
	bool expIsNeg{false};
	auto valptr = value;
	while(length--) {
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
			if(isdigit(c)) {
				mantissa = c - '0';
				state = State::mant;
				break;
			}
			return numberInvalid;

		case State::mant:
			if(c == '.') {
				state = State::frac;
				break;
			}
			if(c == 'e') {
				state = State::exp;
				break;
			}
			if(isdigit(c)) {
				if(mantissa == 0) {
					mantissa = c - '0';
					break;
				}
				auto newMantissa = (mantissa * 10) + c - '0';
				if(newMantissa <= number_t::maxMantissa) {
					mantissa = newMantissa;
					break;
				}
				return numberOverflow;
			}
			return numberInvalid;

		case State::frac:
			if(c == 'e') {
				state = State::exp;
				break;
			}
			if(isdigit(c)) {
				unsigned newMantissa = (mantissa * 10) + c - '0';
				if(newMantissa <= number_t::maxMantissa) {
					mantissa = newMantissa;
					--shift;
					break;
				}
				// Use digit to round value up
				mantissa = (newMantissa + 5) / 10;
				state = State::rounded;
				break;
			}
			return numberInvalid;

		case State::rounded:
			// Discard digit
			if(c == 'e') {
				state = State::exp;
			}
			break;

		case State::exp:
			if(c == '+') {
				break;
			}
			if(c == '-') {
				expIsNeg = true;
				break;
			}
			if(isdigit(c)) {
				exponent = (exponent * 10) + c - '0';
				break;
			}
			return numberInvalid;
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
	// Normalise
	while(mantissa >= 10 && mantissa % 10 == 0) {
		mantissa /= 10;
		++exponent;
	}

	if(abs(exponent) > number_t::maxExponent) {
		return numberOverflow;
	}

	return {isNeg ? -int(mantissa) : int(mantissa), exponent};
}

Number::operator String() const
{
	char buf[32];
	return formatNumber(buf, *this);
}

const char* Number::formatNumber(char* buf, number_t number)
{
	if(number == numberOverflow) {
		strcpy(buf, "OVF");
		return buf;
	}

	auto& v = number;

	// Output mantissa
	auto numptr = buf;
	if(v.mantissa < 0) {
		*numptr++ = '-';
	}
	ultoa(abs(v.mantissa), numptr, 10);
	int mlen = strlen(numptr);

	// Case 1: exponent == 0 (e.g. 0, 1, 2, 3)
	if(v.exponent == 0) {
		return buf;
	}

	// Determine what exponent would be for 'e' form
	int exp = v.exponent + mlen - 1;
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
	if(v.exponent > 0) {
		numptr += mlen;
		memset(numptr, '0', v.exponent);
		numptr[v.exponent] = '\0';
		return buf;
	}

	// Case 2: exponent < 0, insert 0's
	int insertLen = -(mlen + v.exponent);
	if(insertLen >= 0) {
		memmove(numptr + insertLen + 2, numptr, mlen + 1);
		*numptr++ = '0';
		*numptr++ = '.';
		memset(numptr, '0', insertLen);
		return buf;
	}

	// Case 3: insert dp
	numptr += mlen + v.exponent;
	memmove(numptr + 1, numptr, 1 - v.exponent);
	*numptr = '.';
	return buf;
}

size_t Number::printTo(Print& p) const
{
	char buf[32];
	auto str = formatNumber(buf, *this);
	return p.print(str);
}

double Number::asFloat() const
{
	char buf[32];
	auto str = formatNumber(buf, *this);
	return strtod(str, nullptr);
}

int Number::compare(const Number& other) const
{
	if(mantissa == other.mantissa && exponent == other.exponent) {
		return 0;
	}
	// Compare signs
	if(mantissa < 0) {
		if(other.mantissa >= 0) {
			return -1;
		}
	} else if(other.mantissa < 0) {
		return 1;
	}

	// OK, both numbers have same sign
	// Find number of digits in mantissa
	auto exp = adjustedExponent();
	auto otherExp = other.adjustedExponent();
	if(exp < otherExp) {
		return -1;
	}
	if(exp > otherExp) {
		return 1;
	}
	return 0;
}

int Number::adjustedExponent() const
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

} // namespace ConfigDB
