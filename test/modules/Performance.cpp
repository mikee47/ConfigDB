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

			Serial << "FLOAT:" << root.getSimpleFloat() << endl;
			profile(F("getValue [float]"), [&](unsigned) { root.getSimpleFloat(); });

			profile(F("getValue [Print]"), [&](unsigned) {
				MemoryDataStream stream;
				stream << root;
			});
		}

		profile(F("parse number [double]"), [](unsigned) { ConfigDB::Number num(3.141592654e-12); });
		profile(F("parse number [integer]"), [](unsigned) { ConfigDB::Number num(3141593); });
		profile(F("parse number [string]"), [](unsigned) { ConfigDB::Number num("3.141592654e-12"); });
	}
};

void REGISTER_TEST(Performance)
{
	registerGroup<PerformanceTest>();
}
