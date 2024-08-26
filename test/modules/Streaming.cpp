/*
 * Streaming.cpp
 */

#include <ConfigDBTest.h>
#include <Data/Stream/MemoryDataStream.h>
#include <ConfigDB/Json/Format.h>

namespace json
{
// Embedding JSON in C strings is virtual unreadable, just import it
IMPORT_FSTR_LOCAL(update1, PROJECT_DIR "/resource/update1.json")
IMPORT_FSTR_LOCAL(root1, PROJECT_DIR "/resource/root1.json")
} // namespace

class StreamingTest : public TestGroup
{
public:
	StreamingTest() : TestGroup(_F("Streaming"))
	{
		resetDatabase();
	}

	void execute() override
	{
		// Verify initial value
		TestConfig::Root root(database);
		CHECK(!root.getSimpleBool());

		// Setup test stream for reading
		MemoryDataStream mem;
		mem << json::update1;

		// Check streaming into read-only object fails, and data is not changed
		TEST_CASE("Streaming import, read-only")
		{
			REQUIRE(!root.importFromStream(ConfigDB::Json::format, mem));
			REQUIRE(!root.getSimpleBool());
		}

		// Change value
		TEST_CASE("Streaming import, updater")
		{
			if(auto updater = root.update()) {
				mem.seekFrom(0, SeekOrigin::Start);
				REQUIRE(updater.importFromStream(ConfigDB::Json::format, mem));

				REQUIRE_EQ(root.getSimpleBool(), true);
			} else {
				TEST_ASSERT(false);
			}

			mem.clear();
			root.exportToStream(ConfigDB::Json::format, mem);
			String content;
			mem.moveString(content);
			REQUIRE_EQ(content, json::root1);

			mem.clear();
			database.exportToStream(ConfigDB::Json::format, mem);
			mem.moveString(content);
			REQUIRE_EQ(content, json::root1);
		}

		/* Streaming objects */
		resetDatabase();

		TEST_CASE("Streaming object import")
		{
			if(auto updater = root.update()) {
				mem.clear();
				mem << json::update1;
				auto stream = updater.createImportStream(ConfigDB::Json::format);
				REQUIRE_EQ(stream->copyFrom(&mem), json::update1.length());
				REQUIRE_EQ(root.getSimpleBool(), true);
			} else {
				TEST_ASSERT(false);
			}
		}

		TEST_CASE("Streaming object export")
		{
			mem.clear();
			auto stream = root.createExportStream(ConfigDB::Json::format);
			mem.copyFrom(stream.get());
			String content;
			mem.moveString(content);
			REQUIRE_EQ(content, json::root1);
		}

		TEST_CASE("Indexed array update")
		{

		}
	}
};

void REGISTER_TEST(Streaming)
{
	registerGroup<StreamingTest>();
}
