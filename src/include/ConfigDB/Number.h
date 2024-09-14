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
 * This format is aimed to be a simple and transparent way to *store* floating-point numbers
 * without the weird rounding issues of IEEE754 base-2 float / double.
 *
 * Conversion to floating-point format is only done when required by the application.
 * This can be avoided completely by using strings instead of floats to set values.
 *
 * Constant values can also be specified which are converted at compile-time.
 * See `const_number_t`.
 *
 * Integer values can also be specified, but may be subject to rounding if too precise.
 * Very large (or small) values can be used, such as 1.5e25.
 * This would actually be stored as 15e4 `{15, 24}`.
 *
 * It does not need to be computationally efficient, but does have advantages:
 *
 * 		- structure is transparent
 *		- Base-10 operations can be performed efficiently without rounding errors
 *		- Serialising (converting to strings) and de-serialising is consistent and JSON-compatible
 *
 * Some similarity to python's Decimal class, but with restriction on significant digits and exponent.
 *
 * The initial version of this used 24 bits for the mantissa, but 8 bits for the exponent is overkill.
 * Reducing exponent to 6 bits increases precision by 2 bits to give:
 * 	smallest: 1e-31
 *  largest: 33554431e31 (3.3554431e38)
 *
 * Numbers always have a valid representation for ease of use and JSON compatibility.
 * There is no definition for 'NaN' (not a number) or 'infinity.
 * Values are never rounded down to 0, but clipped to the above valid range.
 * Thus convering any number smaller than 1e-31 (but > 0) is stored as 1e-31.
 * Applications can consider number_t::min() and number_t::max() as the 'overflow' values.
 * As with the C++ STL, `lowest()` indicates the most negative value which is equivalent to -max().
 *
 * The mantissa/exponent fields can be manipulated directly if required.
 * For example, to convert terabytes to gigabytes we just subtract 3 from the exponent.
 *
 * An example of custom arithmetic:
 *
 * 			number_t multiplyBy2000(number_t value)
 * 			{
 * 				value.mantissa *= 2;
 * 				value.exponent += 3;
 * 				return value;
 * 			}
 *
 * If there's a risk of over/under flowing in the calculation, do this:
 * 
 * 			number_t multiplyBy2000(number_t value)
 * 			{
 * 				int mantissa = value.mantissa * 2;
 * 				int exponent = value.exponent + 3;
 * 				return number_t::normalise(mantissa, exponent);
 * 			}
 *
 * , avoiding overflow risk, multiplying a number by 2000
 * 			int mantissa = number.mantissa;
 * 			int exponent = number.exponent;
 * 			mantissa *= 2;
 * 			exponent += 3;
 * 			number_t new_number = number_t::normalise(number);
 *
 * 
 *
 * @note This structure is *not* packed to ensure values stored in flash behave correctly.
 */
struct number_t {
	int32_t mantissa : 26;
	int32_t exponent : 6;

	static constexpr unsigned maxMantissa{0x1ffffff}; // 33554431
	static constexpr int maxExponent{0x1f};			  // 31
	static constexpr unsigned maxSignificantDigits{8};
	static constexpr unsigned minBufferSize{17};

	/**
	 * @brief Smallest positive value
	 */
	static constexpr const number_t min()
	{
		return {1, -maxExponent};
	}

	/**
	 * @brief Largest positive value
	 */
	static constexpr const number_t max()
	{
		return {maxMantissa, maxExponent};
	}

	/**
	 * @brief Most negative value
	 */
	static constexpr const number_t lowest()
	{
		return {-int(maxMantissa), maxExponent};
	}

	bool operator==(const number_t& other) const
	{
		return mantissa == other.mantissa && exponent == other.exponent;
	}

	bool operator!=(const number_t& other) const
	{
		return !operator==(other);
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

	static double asFloat(number_t number);

	static int64_t asInt64(number_t number);

	size_t printTo(Print& p) const
	{
		char buf[minBufferSize];
		return p.print(format(buf, *this));
	}

	static int compare(number_t num1, number_t num2);

	/**
	 * @brief Convert number to string
	 * @param buf Buffer of at least number_t::minBufferSize
	 * @param number
	 * @retval char* Points to string, may not be start of *buf*
	 * @note Always use return value to give implementation flexible use of buffer
	 */
	static const char* format(char* buf, number_t number);

	static bool parse(const char* value, unsigned length, number_t& number);

	static number_t parse(const char* value, unsigned length)
	{
		number_t num{};
		parse(value, length, num);
		return num;
	}

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
	static constexpr number_t normalise(unsigned mantissa, int exponent, bool isNeg)
	{
		// A non-zero exponent indicates rounding occurred, so 0 isn't zero!
		if(mantissa == 0 && exponent != 0) {
			return number_t{isNeg ? -1 : 1, -number_t::maxExponent};
		}

		// Discard non-significant digits
		while(mantissa > number_t::maxMantissa * 10) {
			mantissa /= 10;
			++exponent;
		}

		// Round down
		if(mantissa > number_t::maxMantissa && exponent < number_t::maxExponent) {
			auto newMantissa = (mantissa + 5) / 10;
			if(newMantissa > mantissa) {
				// Number was rounded up and gained a digit
				newMantissa /= 10;
			}
			mantissa = newMantissa;
			++exponent;
		}

		// Drop any trailing 0's from mantissa
		while(mantissa >= 10 && mantissa % 10 == 0 && exponent < number_t::maxExponent) {
			mantissa /= 10;
			++exponent;
		}

		// Adjust exponent to keep it in range (without losing precision)
		while(exponent > number_t::maxExponent) {
			mantissa *= 10;
			if(mantissa > number_t::maxMantissa) {
				break;
			}
			--exponent;
		}

		if(exponent > number_t::maxExponent) {
			mantissa = number_t::maxMantissa;
			exponent = number_t::maxExponent;
		} else if(exponent < -number_t::maxExponent) {
			mantissa = 1;
			exponent = -number_t::maxExponent;
		} else {
			mantissa = std::min(mantissa, number_t::maxMantissa);
		}

		return number_t{isNeg ? -int32_t(mantissa) : int32_t(mantissa), exponent};
	}

	static constexpr number_t normalise(double mantissa)
	{
		if(mantissa == 0) {
			return {};
		}

		// Pull significant digits into integer part
		int exponent = 0;
		while(mantissa > -double(number_t::maxMantissa) && mantissa < double(number_t::maxMantissa) &&
			  exponent > -number_t::maxExponent) {
			mantissa *= 10.0;
			--exponent;
		}
		// Reduce integer part to int32 range
		while(mantissa < -0x7fffffff || mantissa > 0x7fffffff) {
			mantissa /= 10;
			++exponent;
		}

		return (mantissa < 0) ? normalise(unsigned(-mantissa), exponent, true)
							  : normalise(unsigned(mantissa), exponent, false);
	}

	static constexpr number_t normalise(int mantissa, int exponent)
	{
		return normalise(abs(mantissa), exponent, mantissa < 0);
	}

	static constexpr number_t normalise(int64_t value)
	{
		bool isNeg{false};
		if(value < 0) {
			isNeg = true;
			value = -value;
		}
		int exponent = 0;
		while(value > 0xffffffffll) {
			value /= 10;
			++exponent;
		}
		return normalise(unsigned(value), exponent, isNeg);
	}
};

static_assert(sizeof(number_t) == 4, "Bad number_t size");

/**
 * @brief Compile-time constant number
 *
 * Must be assigned to a constexpr variable to guarantee compile-time evaluation.
 * For example:
 *
 * 		constexpr const_number_t constnum(3.14);
 *
 * This is typically used to generate numbers stored directly in flash memory,
 * where the structure definition is itself declared constexpr.
 *
 * By inspection, compilers tend to generate runtime code when used normally:
 *
 * 		const_number_t constnum(3.14);
 *
 * This behaviour is because floating-point numbers are not exact representations
 * (integral) so the compiler is erring on the safe side.
 */
struct const_number_t : public number_t {
	const_number_t() = default;

	/**
	 * @brief Computer number from a compile-time constant value
	 */
	constexpr const_number_t(double value) : number_t(normalise(value))
	{
	}
};

/**
 * @brief Base-10 floating-point storage format
 *
 * The base number_t type cannot have constructors so this class includes those
 * for ease of application use.
 *
 * @note Structure is packed to accommodate use in generated class structures
 */
class __attribute__((packed)) Number
{
public:
	Number() = default;

	constexpr Number(const number_t& number) : number(number)
	{
	}

	constexpr Number(const const_number_t& number) : number(number)
	{
	}

	constexpr Number(const Number& number) = default;

	constexpr Number(double value) : number(number_t::normalise(value))
	{
	}

	constexpr Number(int64_t value) : number(number_t::normalise(value))
	{
	}

	constexpr Number(int value) : Number(int64_t(value))
	{
	}

	constexpr Number(unsigned int value) : Number(int64_t(value))
	{
	}

	/**
	 * @brief Parse a number from a string
	 */
	Number(const char* value, unsigned length) : number(number_t::parse(value, length))
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

	bool operator==(const number_t& other) const
	{
		return number == other;
	}

	bool operator!=(const number_t& other) const
	{
		return number != other;
	}

	int compare(const Number& other) const
	{
		return number_t::compare(number, other.number);
	}

	size_t printTo(Print& p) const
	{
		char buf[number_t::minBufferSize];
		return p.print(number_t::format(buf, number));
	}

	double asFloat() const
	{
		return number_t::asFloat(number);
	}

	int64_t asInt64() const
	{
		return number_t::asInt64(number);
	}

	String toString() const
	{
		char buf[number_t::minBufferSize];
		return number_t::format(buf, number);
	}

	explicit operator String() const
	{
		return toString();
	}

	constexpr operator number_t() const
	{
		return number;
	}

private:
	number_t number;
};

} // namespace ConfigDB

using number_t = ConfigDB::number_t;
using const_number_t = ConfigDB::const_number_t;

inline String toString(number_t number)
{
	return ConfigDB::Number(number).toString();
}
