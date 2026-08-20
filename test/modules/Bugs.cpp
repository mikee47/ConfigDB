/*
 * Bugs.cpp
 */

#include <ConfigDBTest.h>
#include <test-bugs.h>

class BugsTest : public TestGroup
{
public:
	BugsTest() : TestGroup(_F("Bugs"))
	{
	}

	void execute() override
	{
		TestBugs bugs("");
		TestBugs::Root root(bugs);

		TEST_CASE("Empty Object misalignment bug")
		{
			/*
				Empty struct is allocated 1 byte by compiler.
				This means calculated positions (by dbgen.py) and those in a struct
				definition will not match.
				Default values are stored as a struct so this check fails.
			*/
			REQUIRE_EQ(root.getGuard(), 555);
		}
	}
};

void REGISTER_TEST(Bugs)
{
	registerGroup<BugsTest>();
}
