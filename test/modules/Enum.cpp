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

		auto& colorType = TestConfigEnum::Root::Colors::itemtype;
		auto& quotientType = TestConfigEnum::Root::Quotients::itemtype;
		auto& smallMapType = TestConfigEnum::Root::SmallMap::itemtype;
		auto& numberMapType = TestConfigEnum::Root::NumberMap::itemtype;

		Serial << "colors[" << colorType.values().length() << "]: " << colorType.values() << endl;
		Serial << "quotients[" << quotientType.values().length() << "]: " << quotientType.values() << endl;
		Serial << "smallMap[" << smallMapType.values().length() << "]: " << smallMapType.values() << endl;
		Serial << "numberMap[" << numberMapType.values().length() << "]: " << numberMapType.values() << endl;

		TestConfigEnum::Root root(db);
		if(auto update = root.update()) {
			for(unsigned i = 0; i < 10; ++i) {
				update.colors.addItem(os_random() % colorType.values().length());
			}
			for(unsigned i = 0; i < 20; ++i) {
				update.quotients.addItem(os_random() % quotientType.values().length());
			}
			for(unsigned i = 0; i < 10; ++i) {
				update.smallMap.addItem(os_random() % smallMapType.values().length());
			}
			for(unsigned i = 0; i < 10; ++i) {
				update.numberMap.addItem(os_random() % numberMapType.values().length());
			}
		}
		Serial << "root: " << root << endl;
	}
};

void REGISTER_TEST(Enum)
{
	registerGroup<EnumTest>();
}
