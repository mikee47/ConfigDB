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
		TestConfig::Root root(database);
		REQUIRE_EQ(root.getSimpleBool(), false);

		// Change value
		if(auto updater = root.beginUpdate()) {
			updater.setSimpleBool(true);
			// REQUIRE_EQ(root.getSimpleBool(), true);
			REQUIRE(updater.commit());

			// A new reader must contain the updated data
			TestConfig::Root root2(database);
			REQUIRE_EQ(root2.getSimpleBool(), true);

			// A second writer must fail
			REQUIRE(!root2.beginUpdate());
		} else {
			TEST_ASSERT(false);
		}
	}

private:
	TestConfig database;
};

void REGISTER_TEST(Sharing)
{
	registerGroup<SharingTest>();
}
