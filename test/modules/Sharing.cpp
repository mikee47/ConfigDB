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

		auto root2 = root;

		// Change value
		if(auto updater = root.beginUpdate()) {
			updater.setSimpleBool(true);
			REQUIRE_EQ(root.getSimpleBool(), true);
			REQUIRE(updater.commit());

			// Nested updates OK
			REQUIRE(root.beginUpdate());
			REQUIRE(root.beginUpdate());
			REQUIRE(root.beginUpdate());

			// A new reader must contain the updated data
			TestConfig::Root root2(database);
			REQUIRE_EQ(root2.getSimpleBool(), true);

			// A second writer must fail
			REQUIRE(!root2.beginUpdate());
		} else {
			TEST_ASSERT(false);
		}

		// Verify store has been unlocked
		REQUIRE(root.beginUpdate());

		// Verify value has changed
		REQUIRE_EQ(root.getSimpleBool(), true);

		// Verify copy we made is stale
		REQUIRE_EQ(root2.getSimpleBool(), false);

		// Get fresh copy which should have changed
		REQUIRE_EQ(TestConfig::Root(database).getSimpleBool(), true);

		// Now try direct
		if(auto updater = TestConfig::Root::Updater(database)) {
			updater.setSimpleBool(false);
		} else {
			TEST_ASSERT(false);
		}

		// Verify value has changed
		REQUIRE_EQ(TestConfig::Root(database).getSimpleBool(), false);

		/* Array */

		if(auto update = root.beginUpdate()) {
			update.intArray.addItem(12);
			REQUIRE_EQ(root.intArray[0], 12);
			update.intArray.removeItem(0);
			REQUIRE_EQ(root.intArray.getItemCount(), 0);
			Serial << root.intArray << endl;
		}

		/* String Array */

		if(auto update = root.beginUpdate()) {
			DEFINE_FSTR_LOCAL(myString, "My String");
			update.stringArray.addItem(myString);
			REQUIRE_EQ(root.stringArray[0], myString);
			update.stringArray.removeItem(0);
			REQUIRE_EQ(root.stringArray.getItemCount(), 0);
			Serial << root.stringArray << endl;
		}
	}

private:
	TestConfig database;
};

void REGISTER_TEST(Sharing)
{
	registerGroup<SharingTest>();
}
