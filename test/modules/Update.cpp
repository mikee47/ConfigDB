/*
 * Update.cpp
 */

#include <ConfigDBTest.h>
#include <test-config-range.h>
#include <test-config-ref.h>
#include <ConfigDB/Json/Format.h>

class UpdateTest : public TestGroup
{
public:
	UpdateTest() : TestGroup(_F("Update"))
	{
		resetDatabase();
	}

	void execute() override
	{
		// Verify initial value
		TestConfig::Root root(database);
		Serial << root << endl;

		TEST_CASE("String")
		{
			String s = root.getSimpleString();
			DEFINE_FSTR_LOCAL(donkey, "donkey")
			REQUIRE_EQ(s, donkey);

			// Change value
			if(auto updater = root.update()) {
				DEFINE_FSTR_LOCAL(banana, "banana")
				updater.setSimpleString(banana);
				REQUIRE_EQ(updater.getSimpleString(), banana);
				updater.setSimpleString(nullptr);
				REQUIRE_EQ(updater.getSimpleString(), donkey);
			} else {
				TEST_ASSERT(false);
			}
		}

		TEST_CASE("Integer")
		{
			REQUIRE_EQ(root.getSimpleInt(), -1);

			// Change value
			if(auto updater = root.update()) {
				// Check clipping
				updater.setSimpleInt(101);
				REQUIRE_EQ(root.getSimpleInt(), 100);
				updater.setSimpleInt(-6);
				REQUIRE_EQ(root.getSimpleInt(), -5);
			} else {
				TEST_ASSERT(false);
			}
		}

		TEST_CASE("Int64")
		{
			TestConfigRange db(F("out/test-range"));
			db.openStoreForUpdate(0)->clear();
			TestConfigRange::Root root(db);
			REQUIRE_EQ(root.getInt64val(), -1);
			if(auto update = root.update()) {
				update.setInt64val(-100000000001LL);
				REQUIRE_EQ(root.getInt64val(), -100000000000LL);
				update.setInt64val(100000000001LL);
				REQUIRE_EQ(root.getInt64val(), 100000000000LL);
			} else {
				TEST_ASSERT(false);
			}
			db.exportToStream(ConfigDB::Json::format, Serial);
			Serial << endl;
		}

		TEST_CASE("Number")
		{
			auto f = root.getSimpleFloat();
			REQUIRE_EQ(String(f), "3.141593");

			// Change value
			if(auto updater = root.update()) {
				const auto newFloat = 12.0e20;
				Serial << newFloat << endl;
				updater.setSimpleFloat(newFloat);
				REQUIRE_EQ(String(root.getSimpleFloat()), F("1.2e21"));
			} else {
				TEST_ASSERT(false);
			}

			TestConfigRange db(F("out/test-range"));
			TestConfigRange::Root::OuterUpdater root(db);
			root.setNumval(0);
			CHECK_EQ(root.getNumval().asFloat(), 0);
			root.setNumval(-2);
			CHECK_EQ(root.getNumval().asFloat(), -1);
			root.setNumval(10.1);
			CHECK_EQ(root.getNumval().asFloat(), 10);
		}
	}
};

void REGISTER_TEST(Update)
{
	registerGroup<UpdateTest>();
}
