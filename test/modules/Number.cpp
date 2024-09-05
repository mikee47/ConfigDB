/*
 * Update.cpp
 */

#include <SmingTest.h>
#include <ConfigDB/Number.h>
#include <FlashString/Array.hpp>

namespace
{
struct TestValue {
	double value;
	char input[20];
	char expected[20];
};

#define TEST_VALUE_MAP(XX)                                                                                             \
	XX(1.00000e10, "1e10")                                                                                             \
	XX(1.00001e10, "1.00001e10")                                                                                       \
	XX(1.0000000000e10, "1e10")                                                                                        \
	XX(1000e123, "1e126")                                                                                              \
	XX(1000e-123, "1e-120")                                                                                            \
	XX(1000e124, "OVF")                                                                                                \
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

} // namespace

class NumberTest : public TestGroup
{
public:
	NumberTest() : TestGroup(_F("Number"))
	{
	}

	void execute() override
	{
		for(auto test : testValues) {
			ConfigDB::Number number(test.input);
			String output(number);

			ConfigDB::Number floatNumber(test.value);

			number_t num = number;

			Serial << "Number " << test.input << ", " << output << ", " << floatNumber << " [" << num.mantissa << ", "
				   << num.exponent << "]" << endl;

			CHECK(number == floatNumber);
			CHECK_EQ(output, test.expected);
		}
	}
};

void REGISTER_TEST(Number)
{
	registerGroup<NumberTest>();
}
