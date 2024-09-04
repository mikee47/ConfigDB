/*
 * Update.cpp
 */

#include <ConfigDBTest.h>
#include <test-config-range.h>
#include <test-config-ref.h>
#include <ConfigDB/Json/Format.h>

namespace
{
IMPORT_FSTR_LOCAL(union_test_root_json, PROJECT_DIR "/resource/union-test-root.json")
}

class UpdateTest : public TestGroup
{
public:
	UpdateTest() : TestGroup(_F("Update"))
	{
		resetDatabase();
	}

	void execute() override
	{
		// TestConfigRef db("test-ref");
		// TestConfigRef::Root root(db);

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
			db.openStore(0, true)->clear();
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
			auto diff = f - 3.141592654;
			Serial << String(f, 8) << ", " << String(diff, 8) << endl;
			REQUIRE(abs(diff) < 0.000001);

			// Change value
			if(auto updater = root.update()) {
				const auto newFloat = 12e6;
				Serial << newFloat << endl;
				updater.setSimpleFloat(newFloat);
				REQUIRE_EQ(root.getSimpleFloat(), newFloat);
			} else {
				TEST_ASSERT(false);
			}
		}
	}
};

void REGISTER_TEST(Update)
{
	registerGroup<UpdateTest>();
}
