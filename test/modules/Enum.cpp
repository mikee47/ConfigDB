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
	}
};

void REGISTER_TEST(Enum)
{
	registerGroup<EnumTest>();
}
