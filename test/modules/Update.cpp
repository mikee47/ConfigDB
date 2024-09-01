/*
 * Update.cpp
 */

#include <ConfigDBTest.h>
#include <test-config-range.h>
#include <test-config-ref.h>
#include <test-config-union.h>
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
		}

		TEST_CASE("Union")
		{
			TestConfigUnion db(F("out/test-union"));
			db.openStore(0, true)->clear();
			Serial << TestConfigUnion::Root(db) << endl;
			using Color = TestConfigUnion::ContainedColor;
			Color::Tag expectedTag{};
			for(unsigned i = 0; i < 4; ++i) {
				TestConfigUnion::Root::Color1 color(db);
				REQUIRE_EQ(color.getTag(), expectedTag);
				Serial << color.getTag() << ": " << color << endl;
				if(i == 3) {
					break;
				}
				auto tag = Color::Tag(i);
				if(auto updater = color.update()) {
					updater.setTag(tag);
					switch(tag) {
					case Color::Tag::RGB:
						updater.getRGB().setBlue(123);
						break;
					case Color::Tag::HSV:
						updater.getHSV().setSaturation(24);
						break;
					case Color::Tag::RAW:
						updater.getRAW().setBlue(456);
						break;
					}
					Serial << "color: " << color << endl;
				}
				expectedTag = tag;
			}
		}
	}
};

void REGISTER_TEST(Update)
{
	registerGroup<UpdateTest>();
}
