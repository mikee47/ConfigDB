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
#include <Data/BitSet.h>

namespace ConfigDB
{
/**
 * @brief Basic definition of base-10 floating point value.
 *
 * 		value = mantissa * 10^exponent
 *
 * Unlike IEEE754 base-2 floats this format is aimed to be a simple and transparent way
 * to *store* floating-point numbers without the weird rounding issues of base-2 regular floats.
 *
 * It does not need to be computationally efficient, but does have advantages:
 *
 * 		- structure is transparent
 *		- Base-10 operations can be performed efficiently without rounding errors
 *		- Serialising (converting to strings) and de-serialising does not 
 *
 * Some similarity to python's Decimal class, but with restriction on significant digits and exponent.
 *
 * The initial version of this used 24 bits for the mantissa, but 8 bits for the exponent is overkill.
 * Reducing exponent to 6 bits increases precision by 2 bits to give:
 * 	smallest: 33554427e-30 (3.3554427e23)
 *  largest: 33554427e30 (3.3554427e37)
 *
 * Unlike base-2 floats the mantissa/exponent fields are useful for manipulating values.
 * For example, to convert terabytes to gigabytes we just subtract 3 from the exponent.
 *
 * Note: We *could* add a +7 offset to the exponent to balance things out:
 * 		smallest: 33554427e-37 (3.3554427e30)
 *  	largest: 33554427e23 (3.3554427e30)
 *
 * But we would then need to visualise the mantissa with an imaginary decimal point after the first digit.
 *
 * @note This structure is *not* packed to ensure values stored in flash behave correctly.
 */
union number_t {
	struct {
		int32_t mantissa : 26;
		int32_t exponent : 6;
	};
	uint32_t value;

	static constexpr unsigned maxMantissa{0x1ffffff - 2};
	static constexpr int maxExponent{0x1e};
	static constexpr unsigned maxSignificantDigits{8};
	static constexpr unsigned minBufferSize{17};

	/**
	 * @brief Smallest positive value
	 */
	static constexpr const number_t min()
	{
		return {{1, -maxExponent}};
	}

	/**
	 * @brief Largest positive value
	 */
	static constexpr const number_t max()
	{
		return {{maxMantissa, maxExponent}};
	}

	/**
	 * @brief Most negative value
	 */
	static constexpr const number_t lowest()
	{
		return {{-int(maxMantissa), maxExponent}};
	}

	static constexpr const number_t invalid()
	{
		return {{0xfffffe, 0x1f}};
	}

	static constexpr const number_t overflow()
	{
		return {{0xffffff, 0x1f}};
	}

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
	enum class Option {
		json,
	};
	using Options = BitSet<uint8_t, Option, 1>;

	Number() = default;

	constexpr Number(const number_t& number) : number(number)
	{
	}

	constexpr Number(const Number& number) = default;

	Number(double value) : number(normalise(value))
	{
	}

	Number(int64_t value) : number(normalise(value))
	{
	}

	Number(int value) : Number(int64_t(value))
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

	String toString(Options options = 0) const;

	explicit operator String() const
	{
		return toString();
	}

	constexpr operator number_t() const
	{
		return number;
	}

	bool isNan() const
	{
		return number == number_t::invalid();
	}

	bool isInf() const
	{
		return number == number_t::overflow();
	}

	/**
	 * @brief Convert number to string
	 * @param buf Buffer of at least number_t::minBufferSize
	 * @param number
	 * @retval char* Points to string, may not be start of *buf*
	 * @note Always use return value to give implementation flexible use of buffer
	 * @note Maybe need some options to indicate how to output NaN, Inf values as JSON doesn't support them
	 */
	static const char* format(char* buf, number_t number, Options options = 0);

	static number_t parse(const char* value, unsigned length);

	/**
	 * @brief Produce a normalised number_t from component values
	 * @param mantissa Mantissa without sign, containing significant digits
	 * @param exponent Exponent
	 * @param isNeg true if mantissa is negative, false if positive
	 * @retval number_t Normalised number value
	 * For example, 1000e9 and 10e11 are normalised to 1e12.
	 * This allows values to be directly compared for equality.
	 *
	 * Mantissa has trailing 0's removed, although may be required for large negative exponents.
	 * For example, 100000e-30 exponent is at limit so cannot be reduced further.
	 *
	 * If necessary, the mantissa is rounded to the nearest whole value.
	 * For example, 3141592654 is rounded up to 31415927.
	 *
	 * If the value is out of range, number_t::overflow is returned.
	 */
	static number_t normalise(unsigned mantissa, int exponent, bool isNeg);

	static number_t normalise(double value);
	static number_t normalise(int64_t value);

private:
	number_t number;
};

class ConstNumber : public Number
{
public:
	using Number::Number;

	constexpr ConstNumber(double value) : Number(normalise(value))
	{
	}

private:
	static constexpr number_t normalise(int mantissa, int exponent)
	{
		// Round mantissa
		while(mantissa < -int(number_t::maxMantissa) || mantissa > int(number_t::maxMantissa)) {
			exponent += ((mantissa + 5) / 10) < mantissa;
			mantissa = (mantissa + 5) / 10;
		}
		// Remove trailing 0's, provided exponent doesn't get too big
		while((mantissa <= -10 || mantissa >= 10) && mantissa % 10 == 0 &&
			  (exponent + 1) < int(number_t::maxExponent)) {
			mantissa /= 10;
			++exponent;
		}
		// Mantissa in range, exponent may not be
		if(mantissa == 0) {
			exponent = 0;
		} else if(exponent > number_t::maxExponent) {
			// Positive infinity
			mantissa = (mantissa < 0) ? -int(number_t::maxMantissa) : number_t::maxMantissa;
			exponent = number_t::maxExponent;
		} else if(exponent < -number_t::maxExponent) {
			// Negative infinity
			mantissa = (mantissa < 0) ? -1 : 1;
			exponent = -number_t::maxExponent;
		}
		return number_t{{.mantissa = int32_t(mantissa), .exponent = exponent}};
	}

	static constexpr number_t normalise(double mantissa)
	{
		// Pull significant digits into integer part
		int exponent = 0;
		while(mantissa > -0x3fffffff && mantissa < 0x3fffffff && exponent > -number_t::maxExponent) {
			mantissa *= 10;
			--exponent;
		}
		// Reduce integer part to int32 range
		while(mantissa < -0x7fffffff || mantissa > 0x7fffffff) {
			mantissa /= 10;
			++exponent;
		}
		return normalise(int(mantissa), exponent);
	}
};

} // namespace ConfigDB

using number_t = ConfigDB::number_t;

inline String toString(const ConfigDB::Number& number)
{
	return number.toString();
}
