/*
 * Update.cpp
 */

#include <ConfigDBTest.h>
#include <test-config-union.h>
#include <ConfigDB/Json/Format.h>

namespace
{
IMPORT_FSTR_LOCAL(union_test_root_json, PROJECT_DIR "/resource/union-test-root.json")
}

class UnionTest : public TestGroup
{
public:
	UnionTest() : TestGroup(_F("Union"))
	{
		resetDatabase();
	}

	void execute() override
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

void REGISTER_TEST(Union)
{
	registerGroup<UnionTest>();
}
