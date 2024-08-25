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
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 0);

		// Verify initial value
		TestConfig::Root root(database);
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);
		REQUIRE_EQ(root.getSimpleBool(), false);

		auto root2 = root;
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1); // root, root2 share the same store

		// Change value
		if(auto updater = root.update()) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root store is now writeable copy, root2 store unchanged
			updater.setSimpleBool(true);
			REQUIRE_EQ(root.getSimpleBool(), true);
			REQUIRE(updater.commit());

			// Nested updates OK
			REQUIRE(root.update());
			REQUIRE(root.update());
			REQUIRE(root.update());

			REQUIRE_EQ(root2.getSimpleBool(), false); // original value

			// A new reader must contain the updated data
			TestConfig::Root root3(database);
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 3); // root/updater, root2 (original), root3 (read cached)
			REQUIRE_EQ(root3.getSimpleBool(), true);		  // updated value

			// A second writer must fail
			REQUIRE(!root2.update());
			REQUIRE(!root3.update());
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 3); // No change
		} else {
			TEST_ASSERT(false);
		}

		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 3); // root (updated), root2 (original), read cached

		// Verify value has changed in original store
		REQUIRE_EQ(root.getSimpleBool(), true);

		// Verify copy we made is stale
		REQUIRE_EQ(root2.getSimpleBool(), false);

		// Verify store has been unlocked
		/*
			This is interesting. root2 store is stale, and no-one else is using it so that gets evicted.
			There is a writeable version available used by root.
			It's not locked, therefore it must contain the latest data so that becomes root2's store.

			Q. Could this be done transparently as part of root commit?
			A. No. Database sees the cached (readable) store is updated but root2 Object holds a pointer to the original store.
			The database doesn't keep track of the referring objects so cannot update them.
			Here, we're explicitly passing `root2` so that's fine, it's what we've asked for.
		*/
		REQUIRE(root2.update());
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root/root2 (updated), read cached
		REQUIRE_EQ(root2.getSimpleBool(), true);		  // root2 now up to date

		// Get fresh copy which should have changed
		REQUIRE_EQ(TestConfig::Root(database).getSimpleBool(), true);
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // No change

		// Now try direct
		if(auto updater = TestConfig::Root::OuterUpdater(database)) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // No change
			updater.setSimpleBool(false);
		} else {
			TEST_ASSERT(false);
		}
		// Updater commits changes, read cache evicted
		REQUIRE_EQ(root.getSimpleBool(), true); // Stale

		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1); // root/root2 (updated), no read cache

		// Verify value has changed
		REQUIRE_EQ(TestConfig::Root(database).getSimpleBool(), false);
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root/root2 (updated), new read cache

		/* Array */

		if(auto update = root.update()) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);
			update.intArray.addItem(12);
			REQUIRE_EQ(root.intArray[0], 12);
			update.intArray[0] = 123;
			REQUIRE_EQ(root.intArray[0], 123);
			Serial << root.intArray << endl;
		}
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);

		/* String Array */

		DEFINE_FSTR_LOCAL(myString, "My String");

		if(auto update = root.update()) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);
			update.stringArray.addItem(myString);
			REQUIRE_EQ(root.stringArray[0], myString);
			update.stringArray[0] = nullptr;
			REQUIRE_EQ(root.stringArray[0], nullptr);
			update.stringArray[0] = myString;
			Serial << root.stringArray << endl;
		}
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);

		/* Object Array */

		if(auto update = root.update()) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);
			auto item = update.objectArray.addItem();
			item.setIntval1(12);
			REQUIRE_EQ(root.objectArray[0].getIntval1(), 12);
			item.setStringval2(myString);
			REQUIRE_EQ(root.objectArray[0].getStringval2(), myString);
			Serial << root.objectArray << endl;
		}
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);

		auto root3 = root;
		auto root4 = root.update();
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);
		REQUIRE(root4);
		auto root5 = root.update();
		REQUIRE(root5);
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);

		auto update = TestConfig::Root::OuterUpdater(database);
		REQUIRE(!update);

		// Async update. Can use `auto upd` but no code completion (at least in vscode)
		auto asyncUpdated = TestConfig::Root(database).update([this](TestConfig::RootUpdater upd) {
			Serial << "ASYNC UPDATE" << endl;
			upd.setSimpleBool(true);
			Serial << upd << endl;
			// Must queue this as `complete()` destroys this class immediately
			System.queueCallback([this]() { complete(); });
		});
		REQUIRE(!asyncUpdated);

		Serial << root << endl;

		pending();
	}

private:
	TestConfig database;
};

void REGISTER_TEST(Sharing)
{
	registerGroup<SharingTest>();
}
