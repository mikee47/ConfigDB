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

/**
 * @brief Basic definition of base-10 floating point value
 * Avoids problems with rounding errors.
 *
 * value = mantissa * 10^exponent
 *
 * Similar in operation to python's Decimal class.
 *
 * @note This structure is *not* packed to ensure values stored in flash behave correctly.
 */
union number_t {
	uint32_t value;
	struct {
		int32_t mantissa : 24;
		int32_t exponent : 8;
	};

	static constexpr uint32_t maxMantissa{0x7fffff};
	static constexpr uint32_t maxExponent{0x7e};

	bool operator==(const number_t& other) const
	{
		return value == other.value;
	}

	bool sign() const
	{
		return mantissa < 0;
	}

	bool operator<(const number_t& other) const
	{
		return compare(*this, other) < 0;
	}

	bool operator>(const number_t& other) const
	{
		return compare(*this, other) > 0;
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

	static int compare(const number_t& num1, const number_t& num2);
};

namespace ConfigDB
{
/**
 * @brief Base-10 floating-point storage format
 * @note Structure is packed to accommodate use in generated class structures
 */
struct __attribute__((packed)) Number {
	number_t number;

	static constexpr number_t invalid{0x7f000000};
	static constexpr number_t overflow{0x7f000001};

	Number() = default;

	constexpr Number(const number_t& number) : number(number)
	{
	}

	constexpr Number(const Number& number) = default;

	Number(double value) : number(parse(value))
	{
	}

	/**
	 * @brief Parse a number from a string
	 */
	Number(const char* value, unsigned length) : number(parse(value, length))
	{
	}

	Number(const char* value) : Number(value, value ? strlen(value) : 0)
	{
	}

	Number(const String& str) : Number(str.c_str(), str.length())
	{
	}

	bool operator<(const Number& other) const
	{
		return number < other.number;
	}

	bool operator>(const Number& other) const
	{
		return number > other.number;
	}

	bool operator==(const Number& other) const
	{
		return number == other.number;
	}

	int compare(const Number& other) const
	{
		return number_t::compare(number, other.number);
	}

	size_t printTo(Print& p) const;

	double asFloat() const;

	operator String() const;

	operator number_t() const
	{
		return number;
	}

	static const char* formatNumber(char* buf, number_t number);

	static number_t parse(double value);
	static number_t parse(const char* value, unsigned length);
	static number_t normalise(unsigned mantissa, int exponent, bool isNeg);
};

} // namespace ConfigDB
