/**
 * ConfigDB/Number.h
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

#pragma once

#include <WString.h>
#include <Print.h>

namespace ConfigDB
{
/**
 * @brief Base-10 floating-point storage format
 * Avoids problems with rounding errors.
 *
 * value = mantissa * 10^exponent
 *
 * Similar in operation to python's Decimal class.
 */
struct __attribute__((packed)) Number {
	int32_t mantissa : 24;
	int32_t exponent : 8;

	static constexpr uint32_t maxMantissa{0xffffff};
	static constexpr uint32_t maxExponent{0x7e};

	Number() = default;

	constexpr Number(const Number& number) = default;

	constexpr Number(int mantissa, int exponent) : mantissa(mantissa), exponent(exponent)
	{
	}

	Number(double value)
	{
		*this = parse(value);
	}

	/**
	 * @brief Parse a number from a string
	 */
	Number(const char* value, unsigned length)
	{
		*this = parse(value, length);
	}

	Number(const char* value) : Number(value, value ? strlen(value) : 0)
	{
	}

	Number(const String& str) : Number(str.c_str(), str.length())
	{
	}

	bool operator<(const Number& other)
	{
		return compare(other) < 0;
	}

	bool operator>(const Number& other)
	{
		return compare(other) > 0;
	}

	bool operator==(const Number& other)
	{
		return compare(other) == 0;
	}

	int compare(const Number& other) const;

	explicit operator uint32_t() const
	{
		return *reinterpret_cast<const uint32_t*>(this);
	}

	/**
	 * @brief Return the adjusted exponent after shifting out the coefficient’s
	 * rightmost digits until only the lead digit remains
	 *
	 * Number('321e+5').adjusted() returns 7.
	 * Used for determining the position of the most significant digit
	 * with respect to the decimal point.
	 */
	int adjustedExponent() const;

	size_t printTo(Print& p) const;

	double asFloat() const;

	operator String() const;

	static const char* formatNumber(char* buf, Number number);

	static Number parse(double value);
	static Number parse(const char* value, unsigned length);
	static Number normalise(unsigned mantissa, int exponent, bool isNeg);
};

constexpr Number numberInvalid{0, 0x7f};
constexpr Number numberOverflow{1, 0x7f};

} // namespace ConfigDB

using number_t = ConfigDB::Number;
