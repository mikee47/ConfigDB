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
	struct {
		int32_t mantissa : 24;
		int32_t exponent : 8;
	};
	uint32_t value;

	static constexpr unsigned maxMantissa{0x7fffff};
	static constexpr unsigned maxExponent{0x7e};
	static constexpr unsigned minBufferSize{16};

	bool operator==(const number_t& other) const
	{
		return value == other.value;
	}

	bool operator!=(const number_t& other) const
	{
		return value != other.value;
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

/**
 * @brief Base-10 floating-point storage format
 * @note Structure is packed to accommodate use in generated class structures
 *
 * Floating-point values are parsed from JSON as strings, which this class then converts
 * into a base-10 representation which has no weird rounding issues.
 * If values are too precise then rounding occurs, but using standard mod-10 arithmetic
 * so is more intuitive.
 *
 * Conversion to internal floating-point format is only done when required by the application.
 * This can be avoided completely by using strings instead of floats to set values.
 *
 * Integer values can also be specified, however they will also be subject to rounding.
 * Large values can be stored, such as 1.5e25.
 * This would actually be stored as 15e24 and applications are free to inspect
 *
 * Applications can 
 */
class __attribute__((packed)) Number
{
public:
	static constexpr number_t invalid{0, 0x7f};
	static constexpr number_t overflow{1, 0x7f};

	Number() = default;

	constexpr Number(const number_t& number) : number(number)
	{
	}

	constexpr Number(const Number& number) = default;

	Number(double value) : number(parse(value))
	{
	}

	Number(int value) : Number(int64_t(value))
	{
	}

	Number(int64_t value) : number(parse(value))
	{
	}

	Number(unsigned int value) : Number(int64_t(value))
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

	bool operator!=(const Number& other) const
	{
		return number != other.number;
	}

	int compare(const Number& other) const
	{
		return number_t::compare(number, other.number);
	}

	size_t printTo(Print& p) const;

	double asFloat() const;
	int64_t asInt64() const;

	String toString() const;

	explicit operator String() const
	{
		return toString();
	}

	operator number_t() const
	{
		return number;
	}

	bool isNan() const
	{
		return number == invalid;
	}

	bool isInf() const
	{
		return number == overflow;
	}

	/**
	 * @brief Convert number to string
	 * @param buf Buffer of at least number_t::minBufferSize
	 * @param number
	 * @retval char* Points to string, may not be start of *buf*
	 * @note Always use return value to give implementation flexible use of buffer
	 * @note Maybe need some options to indicate how to output NaN, Inf values as JSON doesn't support them
	 */
	static const char* format(char* buf, number_t number);

	static number_t parse(double value);
	static number_t parse(int64_t value);
	static number_t parse(const char* value, unsigned length);
	static number_t normalise(unsigned mantissa, int exponent, bool isNeg);

private:
	number_t number;
};

} // namespace ConfigDB

using number_t = ConfigDB::number_t;

inline String toString(const ConfigDB::Number& number)
{
	return number.toString();
}
