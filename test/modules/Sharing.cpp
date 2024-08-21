/*
 * Standard.cpp
 */

#include <SmingTest.h>
#include <test-config.h>
#include <Services/Profiling/MinMaxTimes.h>

class SharingTest : public TestGroup
{
public:
	SharingTest() : TestGroup(_F("Standard")), database("test-config")
	{
		createDirectory(database.getPath());
		auto store = database.openStore(0, true);
		store->clear();
		database.save(*store);
	}

	void execute() override
	{
		// Verify initial value
		TestConfig::Root readRoot(database);
		REQUIRE_EQ(readRoot.getSimpleBool(), false);

		// Change value
		TestConfig::Root writeRoot(database, true);
		writeRoot.setSimpleBool(true);
		REQUIRE_EQ(writeRoot.getSimpleBool(), true);
		REQUIRE_EQ(readRoot.getSimpleBool(), false); // Not yet changed
		REQUIRE(writeRoot.commit());
		REQUIRE_EQ(readRoot.getSimpleBool(), false); // Data is stale

		// A new reader must contain the updated data
		TestConfig::Root readRoot2(database);
		REQUIRE_EQ(readRoot2.getSimpleBool(), true);

		// A second writer should fail
		TestConfig::Root writeRoot2(database, true);
		REQUIRE(!writeRoot2);
	}

private:
	TestConfig database;
};

void REGISTER_TEST(Sharing)
{
	registerGroup<SharingTest>();
}
