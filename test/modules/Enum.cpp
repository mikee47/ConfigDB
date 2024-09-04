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
		db.openStoreForUpdate(0)->clear();

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
		if(auto update = root.update()) {
			update.colors.addItem(Root::ColorsItem(100));
			REQUIRE(update.colors[0] == Root::ColorsItem::blue);
			for(unsigned i = 0; i < 10; ++i) {
				update.colors.addItem(Root::ColorsItem(os_random() % colorType.values().length()));
			}
			for(unsigned i = 0; i < 20; ++i) {
				update.quotients.addItem(Root::QuotientsItem(os_random() % quotientType.values().length()));
			}
			for(unsigned i = 0; i < 10; ++i) {
				update.smallMap.addItem(Root::SmallMapItem(os_random() % smallMapType.values().length()));
			}
			for(unsigned i = 0; i < 10; ++i) {
				update.numberMap.addItem(Root::NumberMapItem(os_random() % numberMapType.values().length()));
			}
		}

		Serial << "root: " << root << endl;
	}
};

void REGISTER_TEST(Enum)
{
	registerGroup<EnumTest>();
}
