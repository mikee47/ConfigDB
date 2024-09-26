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
				updater.resetSimpleInt();
				REQUIRE_EQ(root.getSimpleInt(), -1);
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
			DEFINE_FSTR_LOCAL(floatDefault, "3.1415927")
			auto f = root.getSimpleFloat();
			REQUIRE_EQ(String(f), floatDefault);

			// Change value
			if(auto updater = root.update()) {
				const auto newFloat = 12.0e20;
				Serial << newFloat << endl;
				updater.setSimpleFloat(newFloat);
				REQUIRE_EQ(String(root.getSimpleFloat()), F("1.2e8"));
				updater.resetSimpleFloat();
				REQUIRE_EQ(String(root.getSimpleFloat()), floatDefault);
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

		TEST_CASE("Array defaults")
		{
			DEFINE_FSTR_LOCAL(intArrayDefaults, "[13,28,39,40]")
			DEFINE_FSTR_LOCAL(stringArrayDefaults, "[\"a\",\"b\",\"c\"]")
			REQUIRE_EQ(exportObject(root.intArray), intArrayDefaults);
			REQUIRE_EQ(exportObject(root.stringArray), stringArrayDefaults);

			if(auto upd = root.update()) {
				upd.clear();
				REQUIRE_EQ(upd.intArray.getItemCount(), 0);
				REQUIRE_EQ(upd.stringArray.getItemCount(), 0);
				upd.loadArrayDefaults();
				REQUIRE_EQ(exportObject(upd.intArray), intArrayDefaults);
				REQUIRE_EQ(exportObject(root.stringArray), stringArrayDefaults);
			}
		}

		TEST_CASE("Array iterators")
		{
			if(auto update = root.update()) {
				for(unsigned i = 0; i < 5; ++i) {
					update.intArray.addItem(i);
					update.stringArray.addItem(String(i * 10));
					auto item = update.objectArray.addItem();
					item.setIntval(i * 100);
					item.setStringval(String(i * 1000));
				}
			}

			Serial << F("root.intArray[]");
			for(auto item : root.intArray) {
				Serial << ", " << item;
			}
			Serial << endl;
			for(auto item : root.update().intArray) {
				item = item + 10;
			}
			Serial << root.intArray << endl;

			Serial << F("root.stringArray[]");
			for(auto item : root.stringArray) {
				Serial << ", " << item;
			}
			Serial << endl;
			for(auto item : root.update().stringArray) {
				item = String(item) + F(" updated...");
			}
			Serial << root.stringArray << endl;

			Serial << F("root.objectArray[]");
			for(auto item : root.objectArray) {
				Serial << ", " << item;
			}
			Serial << endl;
			for(auto item : root.update().objectArray) {
				item.setIntval(item.getIntval() * 2);
				String s = item.getStringval() + F(" updated...");
				item.setStringval(s);
			}
			Serial << root.objectArray << endl;
		}

		TEST_CASE("Array as store")
		{
			TestConfig::ArrayStore arrayStore(database);
			if(auto upd = arrayStore.update()) {
				for(unsigned i = 0; i < 10; ++i) {
					upd.addItem(F("hello ") + i);
				}
			}
			Serial << "arrayStore: " << arrayStore << endl;
		}

		TEST_CASE("Object Array as store")
		{
			TestConfig::ObjectArrayStore arrayStore(database);
			if(auto upd = arrayStore.update()) {
				for(unsigned i = 0; i < 10; ++i) {
					upd.addItem().setValue(i);
				}
			}
			Serial << "arrayStore: " << arrayStore << endl;
		}
	}
};

void REGISTER_TEST(Update)
{
	registerGroup<UpdateTest>();
}
