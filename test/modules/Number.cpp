/*
 * Update.cpp
 */

#include <ConfigDB/Number.h>
#include <FlashString/Array.hpp>
#include <SmingTest.h>

namespace
{
struct TestValue {
	double value;
	char input[20];
	char expected[20];
};

#define NUMBER_MIN 1e-31
#define NUMBER_MAX 3.3554431e38
#define NUMBER_LOWEST -3.3554431e38

#define TEST_VALUE_MAP(XX)                                                                                             \
	XX(1e-32, STR(NUMBER_MIN))                                                                                         \
	XX(3.3554431e38, STR(NUMBER_MAX))                                                                                  \
	XX(3.3554432e38, STR(NUMBER_MAX))                                                                                  \
	XX(-3.3554432e38, STR(NUMBER_LOWEST))                                                                              \
	XX(4294967295123.12987, "4.294967e12")                                                                             \
	XX(4.29496729512312987, "4.294967")                                                                                \
	XX(9999999, "9999999")                                                                                             \
	XX(99999999, "1e8")                                                                                                \
	XX(0.9999999, "0.9999999")                                                                                         \
	XX(0.99999999, "1")                                                                                                \
	XX(0.99999999e-20, "1e-20")                                                                                        \
	XX(101.0000001e9, "1.01e11")                                                                                       \
	XX(1000e34, "1e37")                                                                                                \
	XX(33554427e30, "3.3554427e37")                                                                                    \
	XX(-33554427e30, "-3.3554427e37")                                                                                  \
	XX(33554427e-30, "3.3554427e-23")                                                                                  \
	XX(-33554427e-30, "-3.3554427e-23")                                                                                \
	XX(1000e36, STR(NUMBER_MAX))                                                                                       \
	XX(1.00000e10, "1e10")                                                                                             \
	XX(1.00001e10, "1.00001e10")                                                                                       \
	XX(1.0000000000e10, "1e10")                                                                                        \
	XX(-3.141592654e+4, "-31415.927")                                                                                  \
	XX(-3.141592654e+5, "-314159.27")                                                                                  \
	XX(3.141592654e-12, "3.1415927e-12")                                                                               \
	XX(-3.141592654e-12, "-3.1415927e-12")                                                                             \
	XX(-3.141592654e-5, "-3.1415927e-5")                                                                               \
	XX(-3.141592654e-4, "-3.1415927e-4")                                                                               \
	XX(-3.141592654e-3, "-0.0031415927")                                                                               \
	XX(-3.141592654e-2, "-0.031415927")                                                                                \
	XX(-3.141592654e-1, "-0.31415927")                                                                                 \
	XX(-3.141592654e-0, "-3.1415927")                                                                                  \
	XX(3.14, "3.14")                                                                                                   \
	XX(1e-11, "1e-11")                                                                                                 \
	XX(101e-10, "1.01e-8")                                                                                             \
	XX(101e-9, "1.01e-7")                                                                                              \
	XX(101e-8, "1.01e-6")                                                                                              \
	XX(101e-7, "1.01e-5")                                                                                              \
	XX(101e-6, "1.01e-4")                                                                                              \
	XX(101e-5, "0.00101")                                                                                              \
	XX(101e-4, "0.0101")                                                                                               \
	XX(0.001, "0.001")                                                                                                 \
	XX(3141593e-15, "3.141593e-9")                                                                                     \
	XX(0, "0")                                                                                                         \
	XX(1e3, "1000")                                                                                                    \
	XX(10e3, "10000")                                                                                                  \
	XX(10e4, "100000")                                                                                                 \
	XX(101e4, "1.01e6")                                                                                                \
	XX(101e5, "1.01e7")                                                                                                \
	XX(101e6, "1.01e8")                                                                                                \
	XX(101e7, "1.01e9")                                                                                                \
	XX(101e8, "1.01e10")                                                                                               \
	XX(101.0000001e9, "1.01e11")                                                                                       \
	XX(0.00000000001, "1e-11")

#define XX(value, str) {value, STR(value), str},
DEFINE_FSTR_ARRAY_LOCAL(testValues, TestValue, TEST_VALUE_MAP(XX))
#undef XX

struct CompareValue {
	char a[16];
	char b[16];
	int compare;
};

#define COMPARE_VALUE_MAP(XX)                                                                                          \
	XX(1000000e9, 1000001e9, -1)                                                                                       \
	XX(10000000e9, 10000001e9, -1)                                                                                     \
	XX(100000000e9, 100000001e9, 0)                                                                                    \
	XX(-2, -1, -1)                                                                                                     \
	XX(-1, -0.9, -1)                                                                                                   \
	XX(-0.9, 0, -1)                                                                                                    \
	XX(0, 0.9, -1)                                                                                                     \
	XX(0.9, 1, -1)                                                                                                     \
	XX(1, 2, -1)                                                                                                       \
	XX(0.9, -0.9, 1)                                                                                                   \
	XX(1e-100, 0, 1)                                                                                                   \
	XX(1e-10, 10e-10, -1)                                                                                              \
	XX(0, 1, -1)                                                                                                       \
	XX(0, 0, 0)                                                                                                        \
	XX(1, 1, 0)                                                                                                        \
	XX(10, 1, 1)                                                                                                       \
	XX(1e-10, 10e-10, -1)                                                                                              \
	XX(1e1, 1e-1, 1)                                                                                                   \
	XX(1e-100, 0, 1)

#define XX(a, b, c) {STR(a), STR(b), c},
DEFINE_FSTR_ARRAY_LOCAL(compareValues, CompareValue, COMPARE_VALUE_MAP(XX))
#undef XX

template <typename T> struct IntValue {
	T value;
	char input[20];
	char expected[20];
};

#define INT_VALUE_MAP(XX)                                                                                              \
	XX(0x7fffffff, "2.1474836e9")                                                                                      \
	XX(2147483549, "2.1474835e9")                                                                                      \
	XX(2147483550, "2.1474836e9")                                                                                      \
	XX(2000000000, "2e9")

#define UINT_VALUE_MAP(XX)                                                                                             \
	XX(0x7fffffff, "2.147484e9")                                                                                       \
	XX(0xffffffff, "4.294967e9")                                                                                       \
	XX(4000000000, "4e9")

#define INT64_VALUE_MAP(XX)                                                                                            \
	XX(0xffffffff, "4.294967e9")                                                                                       \
	XX(0xffffffffffffull, "2.81475e14")                                                                                \
	XX(400000000000, "4e11")                                                                                           \
	XX(200000000000, "2e11")

#define XX(a, b) {a, STR(a), b},
DEFINE_FSTR_ARRAY_LOCAL(intValues, IntValue<int>, INT_VALUE_MAP(XX))
DEFINE_FSTR_ARRAY_LOCAL(uintValues, IntValue<unsigned>, UINT_VALUE_MAP(XX))
DEFINE_FSTR_ARRAY_LOCAL(int64Values, IntValue<int64_t>, INT64_VALUE_MAP(XX))
#undef XX

// Use library function to print number (may not be available in newlib builds)
String floatToStr(double value)
{
	char buf[64];
	sprintf(buf, "%.7lg", value);
	return buf;
}

} // namespace

class NumberTest : public TestGroup
{
public:
	NumberTest() : TestGroup(_F("Number"))
	{
	}

	void execute() override
	{
		TEST_CASE("Parsing and printing")
		{
			for(auto test : testValues) {
				ConfigDB::Number number(test.input);
				String output(number);

				ConfigDB::Number floatNumber(test.value);

				number_t num = number;

				Serial << "Number " << test.input << ", " << output << ", " << floatNumber << " [" << num.mantissa
					   << ", " << num.exponent << "]" << endl;

				CHECK_EQ(number, floatNumber);
				if(output != test.expected) {
					ConfigDB::Number xxnum(test.input);
					Serial << xxnum << endl;
				}
				CHECK_EQ(output, test.expected);
			}
		}

		TEST_CASE("Integers")
		{
			for(auto test : intValues) {
				ConfigDB::Number number(test.value);
				auto input = strtol(test.input, nullptr, 0);
				int64_t output = strtod(test.expected, nullptr);
				Serial << "INT " << test.input << ", " << input << ": " << number << ", " << number.asInt64() << ", "
					   << String(number.asInt64(), HEX) << ", " << output << ", " << String(output, HEX) << endl;
				REQUIRE_EQ(String(test.expected), toString(number));
			}
			for(auto test : uintValues) {
				ConfigDB::Number number(test.value);
				auto input = strtoul(test.input, nullptr, 0);
				Serial << "UINT " << test.input << ", " << input << ": " << number << ", " << number.asInt64() << ", "
					   << String(number.asInt64(), HEX) << endl;
			}
			for(auto test : int64Values) {
				ConfigDB::Number number(test.value);
				auto input = strtoll(test.input, nullptr, 0);
				Serial << "INT64 " << test.input << ", " << input << ": " << number << ", " << number.asInt64() << ", "
					   << String(number.asInt64(), HEX) << endl;
			}
		}

		double v = 1 / 14.0;
		Serial << floatToStr(v) << ", " << ConfigDB::Number(v) << endl;

		// return;

		TEST_CASE("Compare")
		{
			for(auto test : compareValues) {
				Serial << "compare(" << test.a << ", " << test.b << ")" << endl;
				CHECK_EQ(ConfigDB::Number(test.a).compare(test.b), test.compare);
				CHECK_EQ(ConfigDB::Number(test.b).compare(test.a), (~test.compare) + 1);
			}
		}

		TEST_CASE("String length")
		{
			size_t maxLength{0};

			auto check = [&](unsigned exponent, double value) {
				ConfigDB::Number number(value);
				int error = (1e7 * (1 - value / number.asFloat())) + 0.5;
				Serial << exponent << ": " << floatToStr(value) << ", " << number << ", "
					   << floatToStr(number.asFloat()) << ", " << floatToStr(error) << endl;
				maxLength = std::max(maxLength, String(number).length());
			};

			// const double initval{1.1111111};
			const double initval{6.666666};

			double value = initval;
			for(unsigned i = 0; i <= ConfigDB::number_t::maxExponent; ++i) {
				check(i, value);
				value /= 10.0;
			}
			value = initval;
			for(unsigned i = 0; i <= ConfigDB::number_t::maxExponent; ++i) {
				check(i, value);
				value *= 10.0;
			}

			Serial << "Max length = " << maxLength << endl;
		}

		if(0) {
			// constexpr number_t expected{4294967, 3};
			ConfigDB::Number num = ConfigDB::Number::normalise(0xffffffff, 0, 0);
			REQUIRE_EQ(num.asInt64(), 4294967000ull);
			num = ConfigDB::Number::normalise(429496729ul, 0, 0);
			REQUIRE_EQ(num.asInt64(), 429496700ull);
			num = ConfigDB::Number::normalise(429496699ul, 0, 0);
			REQUIRE_EQ(num.asInt64(), 429496700ull);
			num = ConfigDB::Number::normalise(429496644ul, 0, 0);
			REQUIRE_EQ(num.asInt64(), 429496600ull);
			num = ConfigDB::Number::normalise(199999999ul, 0, 0);
			REQUIRE_EQ(num.asInt64(), 200000000ull);

#ifdef ARCH_HOST
			auto check = [&](uint64_t m) {
				ConfigDB::Number num = ConfigDB::Number::normalise(m, 0, 0);
				unsigned mult = 1;
				int64_t expected = m;
				while(expected > number_t::maxMantissa) {
					expected = (expected + 5) / 10;
					mult *= 10;
				}
				expected *= mult;
				if(expected == num.asInt64()) {
					return;
				}
				Serial << "Mantissa " << m << " expected " << expected << ", got " << num.asInt64() << endl;
				TEST_ASSERT(false);
			};

			// Brute-force check values at extremes
			for(uint64_t m = 0xffff0000ull; m <= 0xffffffffull; ++m) {
				check(m);
			}
			for(unsigned m = 0; m <= number_t::maxMantissa * 2; ++m) {
				check(m);
			}
#endif
		}

		TEST_CASE("constexpr")
		{
			using Number = ConfigDB::Number;

			constexpr Number num1 = const_number_t(-5000e-14);
			REQUIRE_EQ(num1, -5e-11);

			constexpr Number num2 = const_number_t(1234567890123LL);
			REQUIRE_EQ(num2, 12345679e5);

			constexpr Number num3 = const_number_t(9223372036854775807LL);
			REQUIRE_EQ(num3, 9223372e12);

			Serial << "num4 = " << const_number_t(124e12) << endl;
			Serial << "num4 = " << const_number_t(124e120) << endl;

			REQUIRE_EQ(toString(const_number_t(0x7fffffff)), "2.1474836e9");
			REQUIRE_EQ(toString(const_number_t(2147483549)), "2.1474835e9");
			REQUIRE_EQ(toString(const_number_t(2147483550)), "2.1474836e9");

			REQUIRE_EQ(toString(const_number_t(3.3554432e38)), STR(NUMBER_MAX));
			REQUIRE_EQ(toString(const_number_t(-3.3554432e38)), STR(NUMBER_LOWEST));
		}
	}
};

void REGISTER_TEST(Number)
{
	registerGroup<NumberTest>();
}
