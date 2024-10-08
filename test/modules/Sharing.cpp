/*
 * Sharing.cpp
 */

#include <ConfigDBTest.h>

class SharingTest : public TestGroup
{
public:
	SharingTest() : TestGroup(_F("Sharing"))
	{
		resetDatabase();
	}

	void execute() override
	{
		// One write reference held by database
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1);

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

		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root (updated), root2 (original), read cache reset

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
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1); // root/root2 (updated)
		REQUIRE_EQ(root2.getSimpleBool(), true);		  // root2 now up to date

		// Get fresh copy which should have changed
		REQUIRE_EQ(TestConfig::Root(database).getSimpleBool(), true);
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 1); // root/root2/root3

		// Now try direct
		if(auto updater = TestConfig::Root::OuterUpdater(database)) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root/root2/root3, updater/write cache
			updater.setSimpleBool(false);
		} else {
			TEST_ASSERT(false);
		}
		// Updater commits changes, read cache evicted
		REQUIRE_EQ(root.getSimpleBool(), true); // Stale

		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root/root2/root3, write cache

		// Verify value has changed
		REQUIRE_EQ(TestConfig::Root(database).getSimpleBool(), false);
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root/root2/root3, read cache

		/* Array */

		if(auto update = root.update()) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root updater, root2/root3
			update.intArray.clear();
			update.intArray.addItem(12);
			REQUIRE_EQ(root.intArray[0], 12);
			update.intArray[0] = 123;
			REQUIRE_EQ(root.intArray[0], 100);
			Serial << root.intArray << endl;
		}
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root, root2/root3

		/* String Array */

		DEFINE_FSTR_LOCAL(myString, "My String");

		if(auto update = root.update()) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2); // root updater, root2/root3
			update.stringArray.clear();
			update.stringArray.addItem(myString);
			REQUIRE_EQ(root.stringArray[0], myString);
			REQUIRE(root.stringArray.contains(myString));
			REQUIRE_EQ(root.stringArray.indexOf(myString), 0);
			update.stringArray[0] = nullptr;
			REQUIRE(!root.stringArray.contains(myString));
			REQUIRE_EQ(root.stringArray[0], "abc");
			update.stringArray[0] = myString;
			Serial << root.stringArray << endl;
		}
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2);

		/* Object Array */

		if(auto update = root.update()) {
			CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2);
			auto item = update.objectArray.addItem();
			item.setIntval(12);
			REQUIRE_EQ(root.objectArray[0].getIntval(), 12);
			item.setStringval(myString);
			REQUIRE_EQ(root.objectArray[0].getStringval(), myString);
			Serial << root.objectArray << endl;
		}
		CHECK_EQ(ConfigDB::Store::getInstanceCount(), 2);

		// Bring root2 up to date, discard stale Store instance
		root2.update();
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
			complete();
		});
		REQUIRE(!asyncUpdated);

		Serial << root << endl;

		pending();
	}
};

void REGISTER_TEST(Sharing)
{
	registerGroup<SharingTest>();
}
