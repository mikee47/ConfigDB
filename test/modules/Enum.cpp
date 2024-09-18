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

		auto& colorType = Root::Colors::itemtype;
		auto& quotientType = Root::Quotients::itemtype;
		auto& smallMapType = Root::SmallMap::itemtype;
		auto& numberMapType = Root::NumberMap::itemtype;

		Serial << "colors[" << colorType.values().length() << "]: " << colorType.values() << endl;
		Serial << "quotients[" << quotientType.values().length() << "]: " << quotientType.values() << endl;
		Serial << "smallMap[" << smallMapType.values().length() << "]: " << smallMapType.values() << endl;
		Serial << "numberMap[" << numberMapType.values().length() << "]: " << numberMapType.values() << endl;

		TestConfigEnum::Root root(db);

		DEFINE_FSTR_LOCAL(quotientsDefault, "[37,15,2500]")
		REQUIRE_EQ(exportObject(root.quotients), quotientsDefault);

		DEFINE_FSTR_LOCAL(colorsDefault, "[\"blue\",\"blue\",\"green\"]")
		REQUIRE_EQ(exportObject(root.colors), colorsDefault);

		if(auto update = root.update()) {
			update.colors.addItem(Root::Color(100));
			REQUIRE(update.colors[0] == Root::Color::blue);
			for(unsigned i = 0; i < 10; ++i) {
				update.colors.addItem(Root::Color(os_random() % colorType.values().length()));
			}
			for(unsigned i = 0; i < 20; ++i) {
				update.quotients.addItem(Root::Quotient(os_random() % quotientType.values().length()));
			}
			for(unsigned i = 0; i < 10; ++i) {
				update.smallMap.addItem(os_random() % smallMapType.values().length());
			}
			for(unsigned i = 0; i < 10; ++i) {
				update.numberMap.addItem(os_random() % numberMapType.values().length());
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
