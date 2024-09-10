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

		profile(F("setValue [int]"), [](unsigned value) {
			TestConfig::Root::OuterUpdater root(database);
			root.setSimpleInt(value);
			root.clearDirty();
		});

		constexpr const double testFloat = PI;
		profile(F("setValue [float]"), [](unsigned) {
			TestConfig::Root::OuterUpdater root(database);
			root.setSimpleFloat(testFloat);
			root.clearDirty();
		});

		profile(F("Set Value + commit"), [](unsigned value) {
			TestConfig::Root::OuterUpdater root(database);
			root.setSimpleInt(value);
		});

		Serial << _F("Evaluating getValue ...") << endl;

		// Cache store so it doesn't skew results
		Serial << "INT:" << TestConfig::Root(database).getSimpleInt() << endl;
		profile(F("getValue [int]"), [](unsigned) {
			TestConfig::Root root(database);
			root.getSimpleInt();
		});

		Serial << "FLOAT:" << TestConfig::Root(database).getSimpleFloat() << endl;
		profile(F("getValue [float]"), [](unsigned) {
			TestConfig::Root root(database);
			root.getSimpleFloat();
		});

		profile(F("getValue [Print]"), [](unsigned) {
			MemoryDataStream stream;
			TestConfig::Root root(database);
			stream << root;
		});
	}
};

void REGISTER_TEST(Performance)
{
	registerGroup<PerformanceTest>();
}
