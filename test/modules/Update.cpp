/*
 * Update.cpp
 */

#include <ConfigDBTest.h>
#include <test-config-range.h>
#include <test-config-ref.h>
#include <test-config-union.h>
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

		testUnion();

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

	void testUnion()
	{
		TestConfigUnion db(F("out/test-union"));
		db.openStore(0, true)->clear();
		using Color = TestConfigUnion::ContainedColor;

		TEST_CASE("Union")
		{
			Serial << TestConfigUnion::Root(db) << endl;
			Color::Tag expectedTag{};
			for(unsigned i = 0; i < 5; ++i) {
				TestConfigUnion::Root::Color1 color(db);
				REQUIRE_EQ(color.getTag(), expectedTag);
				Serial << color.getTag() << ": " << color << endl;
				if(i == 4) {
					break;
				}
				auto tag = Color::Tag(i);
				if(auto updater = color.update()) {
					updater.setTag(tag);
					switch(tag) {
					case Color::Tag::RGB:
						updater.asRGB().setBlue(123);
						break;
					case Color::Tag::HSV:
						updater.asHSV().setSaturation(24);
						break;
					case Color::Tag::RAW:
						updater.asRAW().setBlue(456);
						break;
					case Color::Tag::ColorList:
						updater.asColorList().addItem().toHSV().setHue(123);
						break;
					}
					Serial << "color: " << color << endl;
				}
				expectedTag = tag;
			}
		}

		TEST_CASE("ObjectArray[Union]")
		{
			TestConfigUnion::Root::Colors colors(db);
			if(auto update = colors.update()) {
				auto hsv = update.addItemHSV();
				hsv.setHue(10);
				hsv.setSaturation(20);
				hsv.setValue(30);

				auto rgb = update.addItemRGB();
				rgb.setRed(1);
				rgb.setGreen(2);
				rgb.setBlue(3);

				auto raw = update.insertItemRAW(1);
				raw.setRed(11);
				raw.setGreen(22);
				raw.setBlue(33);
				raw.setWarmWhite(44);
				raw.setColdWhite(55);

				Serial << "colors: " << colors << endl;

				auto item = update.addItem();
				item.toRGB().setBlue(188);

				item = update.addItem();
				item.toHSV().setSaturation(55);

				item = update.addItem();
				raw = item.toRAW();
				raw.setRed(5);
				raw.setGreen(6);
				raw.setBlue(7);
				raw.setWarmWhite(77);
				raw.setColdWhite(18);

				item = update.addItem();
				auto list = item.toColorList();

				list.addItem().setTag(Color::Tag::HSV);
				list.addItem().toColorList().addItem();
				list.addItemRAW();
				Serial << "list: " << list << endl;
				Serial << "colors: " << colors << endl;
			}
		}

		TEST_CASE("Union with empty choice")
		{
			// e.g. used as placeholder
			TestConfigUnion::Root::Color3::OuterUpdater color3(db);
			using Tag = TestConfigUnion::Root::Color3::Tag;
			REQUIRE_EQ(color3.getTag(), Tag::Last);
			auto color = color3.toColor();
			color.toRAW().setWarmWhite(234);
		}

		TestConfigUnion::Root root(db);
		String json = exportObject(root);
		CHECK_EQ(json, union_test_root_json);
	}
};

void REGISTER_TEST(Update)
{
	registerGroup<UpdateTest>();
}
