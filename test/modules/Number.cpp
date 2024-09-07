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

#define TEST_VALUE_MAP(XX)                                                                                             \
	XX(101.0000001e9, "1.01e11")                                                                                       \
	XX(1000e124, "1e127")                                                                                              \
	XX(1.00000e10, "1e10")                                                                                             \
	XX(1.00001e10, "1.00001e10")                                                                                       \
	XX(1.0000000000e10, "1e10")                                                                                        \
	XX(1000e123, "1e126")                                                                                              \
	XX(1000e-123, "1e-120")                                                                                            \
	XX(1000e124, "1e127")                                                                                              \
	XX(3.141592654e126, "3.141593e126")                                                                                \
	XX(3.141592654e132, "3.141593e132")                                                                                \
	XX(3.141592654e133, "OVF")                                                                                         \
	XX(-3.141592654e+4, "-31415.93")                                                                                   \
	XX(-3.141592654e+5, "-314159.3")                                                                                   \
	XX(-3.141592654e-5, "-3.141593e-5")                                                                                \
	XX(-3.141592654e-4, "-3.141593e-4")                                                                                \
	XX(-3.141592654e-3, "-0.003141593")                                                                                \
	XX(-3.141592654e-2, "-0.03141593")                                                                                 \
	XX(-3.141592654e-1, "-0.3141593")                                                                                  \
	XX(-3.141592654e-0, "-3.141593")                                                                                   \
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
	XX(0.00000000001, "1e-11")                                                                                         \
	XX(3.141592654e-12, "3.141593e-12")                                                                                \
	XX(-3.141592654e-12, "-3.141593e-12")                                                                              \
	XX(-3.141592654e-5, "-3.141593e-5")

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
	XX(10000000e9, 10000001e9, 0)                                                                                      \
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

				auto num = number.number;

				Serial << "Number " << test.input << ", " << output << ", " << floatNumber << " [" << num.mantissa
					   << ", " << num.exponent << "]" << endl;

				CHECK_EQ(number, floatNumber);
				CHECK_EQ(output, test.expected);
			}
		}

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
	}
};

void REGISTER_TEST(Number)
{
	registerGroup<NumberTest>();
}
