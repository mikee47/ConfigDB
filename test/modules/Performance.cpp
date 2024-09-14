#include <ConfigDBTest.h>
#include <Services/Profiling/MinMaxTimes.h>
#include <Data/Stream/MemoryDataStream.h>

namespace
{
using Callback = Delegate<void(unsigned round)>;

void __noinline profile(const String& title, Callback callback)
{
	const int rounds = 32;

	Profiling::MicroTimes times(title);
	for(int i = 0; i < rounds; i++) {
		times.start();
		callback(i);
		times.update();
	}
	Serial << times << endl;
}

} // namespace

class PerformanceTest : public TestGroup
{
public:
	PerformanceTest() : TestGroup(_F("Performance"))
	{
	}

	void execute() override
	{
		Serial << endl << _F("** Performance **") << endl;

		Serial << _F("Evaluating load times ...") << endl;
		// Load same cache multiple times
		profile(F("Verify load caching"), [](unsigned) { database.openStore(0); });

		// Load different stores in sequence to bypass caching
		profile(F("Load all stores"),
				[](unsigned i) { auto store = database.openStore(i % database.typeinfo.storeCount); });

		Serial << _F("Evaluating setValue / commit ...") << endl;

		{
			TestConfig::Root::OuterUpdater root(database);

			profile(F("setValue [int]"), [&](unsigned value) { root.setSimpleInt(value); });

			constexpr const double testFloat = PI;
			profile(F("setValue [number float]"), [&](unsigned) { root.setSimpleFloat(testFloat); });

			profile(F("setValue [number int]"), [&](unsigned value) { root.setSimpleFloat(value * 100); });

			profile(F("setValue [number string]"), [&](unsigned) { root.setSimpleFloat("3.141592654"); });

			profile(F("Set Value + commit"), [&](unsigned value) {
				root.setSimpleInt(value);
				root.commit();
			});
		}

		Serial << _F("Evaluating getValue ...") << endl;

		{
			TestConfig::Root root(database);

			Serial << "INT:" << root.getSimpleInt() << endl;
			profile(F("getValue [int]"), [&](unsigned) { root.getSimpleInt(); });

			profileGetNumber();

			root.update().setSimpleFloat(number_t::min());
			profileGetNumber();

			root.update().setSimpleFloat(number_t::max());
			profileGetNumber();

			profile(F("getValue [Print]"), [&](unsigned) {
				MemoryDataStream stream;
				stream << root;
			});
		}

		double floatval = os_random() * 1234.0;
		profile(F("Number(double)"), [this, floatval](unsigned) { staticNumber = ConfigDB::Number(floatval); });
		int64_t intval = os_random() * 1234LL;
		profile(F("Number(integer)"), [this, intval](unsigned) { staticNumber = ConfigDB::Number(intval); });

		profile(F("Number(constexpr double)"), [this](unsigned) {
			constexpr const_number_t num(3.141592654e-12);
			staticNumber = num;
		});

		profile(F("Number(constexpr integer)"), [this](unsigned) {
			constexpr const_number_t num(3141593);
			staticNumber = num;
		});

		profile(F("Number(String)"), [this](unsigned) { staticNumber = ConfigDB::Number("3.141592654e-12"); });
	}

	void profileGetNumber()
	{
		TestConfig::Root root(database);
		Serial << "NUM:" << root.getSimpleFloat() << endl;
		profile(F("getValue [number]"), [&](unsigned) { root.getSimpleFloat(); });
		profile(F("getValue [number as int]"), [&](unsigned) { root.getSimpleFloat().asInt64(); });
		profile(F("getValue [number as float]"), [&](unsigned) { root.getSimpleFloat().asFloat(); });
	}

	number_t staticNumber;
};

void REGISTER_TEST(Performance)
{
	registerGroup<PerformanceTest>();
}
