/*
 * Update.cpp
 */

#include <ConfigDBTest.h>
#include <test-config-enum.h>

class EnumTest : public TestGroup
{
public:
	EnumTest() : TestGroup(_F("Enum"))
	{
	}

	void execute() override
	{
		TestConfigEnum db(F("out/test-enum"));
		db.openStoreForUpdate(0)->resetToDefaults();

		using Root = TestConfigEnum::Root;

		auto& colorType = Root::Colors::itemType;
		auto& quotientType = Root::Quotients::itemType;
		auto& smallMapType = Root::SmallMap::itemType;
		auto& numberMapType = Root::NumberMap::itemType;

		Serial << "colors[" << colorType.values().length() << "]: " << colorType.values() << endl;
		Serial << "quotients[" << quotientType.values().length() << "]: " << quotientType.values() << endl;
		Serial << "smallMap[" << smallMapType.values().length() << "]: " << smallMapType.values() << endl;
		Serial << "numberMap[" << numberMapType.values().length() << "]: " << numberMapType.values() << endl;

		// Verify `toString()` functions
		Serial << _F("toString(Color::blue): ") << TestConfigEnum::Color::blue << endl;

		TestConfigEnum::Root root(db);

		DEFINE_FSTR_LOCAL(quotientsDefault, "[37,15,2500]")
		REQUIRE_EQ(exportObject(root.quotients), quotientsDefault);

		DEFINE_FSTR_LOCAL(colorsDefault, "[\"blue\",\"blue\",\"green\"]")
		REQUIRE_EQ(exportObject(root.colors), colorsDefault);

		if(auto update = root.update()) {
			update.colors.addItem(TestConfigEnum::Color(100));
			REQUIRE(update.colors[0] == TestConfigEnum::Color::blue);
			for(unsigned i = 0; i < 10; ++i) {
				update.colors.addItem(TestConfigEnum::ColorType::range.random());
			}
			for(unsigned i = 0; i < 20; ++i) {
				update.quotients.addItem(Root::QuotientType::range.random());
			}
			for(unsigned i = 0; i < 10; ++i) {
				update.smallMap.addItem(root.smallMap.itemType.range.random());
			}
			for(unsigned i = 0; i < 10; ++i) {
				update.numberMap.addItem(root.numberMap.itemType.range.random());
			}
		}

		Serial << "root: " << root << endl;

		TestConfigEnum::Root::OuterUpdater lock(db);
		auto asyncUpdated = TestConfigEnum::Root(db).update([](auto) { Serial << "ASYNC UPDATE" << endl; });
		REQUIRE(!asyncUpdated);

		TEST_CASE("Ranges")
		{
			using Color = TestConfigEnum::Color;
			constexpr auto& range = TestConfigEnum::ColorType::range;
			Color badColor = Color(1000);
			REQUIRE(!range.contains(badColor));
			REQUIRE_EQ(range.clip(badColor), Color::blue);

			// Because enums are defined using `uint8_t` storage, clipping doesn't work as expected here
			badColor = Color(-1);
			REQUIRE(!range.contains(badColor));
			REQUIRE_EQ(range.clip(badColor), Color::blue);
		}

		/*
		 * RAW enums without ctype override
		 */
		TEST_CASE("Raw enums")
		{
			// These properties contain a value index, not the value itself
			auto numIndex = root.getNum();
			int num = TestConfigEnum::numType.values()[numIndex];
#ifdef SMING_RELEASE
			REQUIRE_EQ(numIndex, 4);
			REQUIRE_EQ(num, 25);
#else
			REQUIRE_EQ(numIndex, 5);
			REQUIRE_EQ(num, 45);
#endif

			auto wordIndex = root.getWord();
			String word = TestConfigEnum::wordType.values()[wordIndex];
			REQUIRE_EQ(wordIndex, 2);
			REQUIRE_EQ(word, "brown");
		}
	}
};

void REGISTER_TEST(Enum)
{
	registerGroup<EnumTest>();
}
