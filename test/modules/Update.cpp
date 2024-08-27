/*
 * Update.cpp
 */

#include <ConfigDBTest.h>

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
};

void REGISTER_TEST(Update)
{
	registerGroup<UpdateTest>();
}
