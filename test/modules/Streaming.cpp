/*
 * Streaming.cpp
 */

#include <ConfigDBTest.h>
#include <Data/Stream/MemoryDataStream.h>
#include <ConfigDB/Json/Format.h>

class StreamingTest : public TestGroup
{
public:
	StreamingTest() : TestGroup(_F("Streaming"))
	{
		resetDatabase();
	}

	void execute() override
	{
		DEFINE_FSTR_LOCAL(jsonText, "{\"simple-bool\": true}");
		DEFINE_FSTR_LOCAL(rootContent,
						  "{\"int array\":[],\"string array\":[],\"object array\":[],\"simple-bool\":true}")

		// Verify initial value
		TestConfig::Root root(database);
		CHECK(!root.getSimpleBool());

		// Setup test stream for reading
		MemoryDataStream mem;
		mem << jsonText;

		// Check streaming into read-only object fails, and data is not changed
		REQUIRE(!root.importFromStream(ConfigDB::Json::format, mem));
		REQUIRE(!root.getSimpleBool());

		// Change value
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
		REQUIRE_EQ(content, rootContent);

		mem.clear();
		database.exportToStream(ConfigDB::Json::format, mem);
		mem.moveString(content);
		REQUIRE_EQ(content, rootContent);

		/* Streaming objects */
		resetDatabase();

		TEST_CASE("Streaming object import")
		{
			if(auto updater = root.update()) {
				mem.clear();
				mem << jsonText;
				auto stream = updater.createImportStream(ConfigDB::Json::format);
				REQUIRE_EQ(stream->copyFrom(&mem), jsonText.length());
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
			REQUIRE_EQ(content, rootContent);
		}
	}
};

void REGISTER_TEST(Streaming)
{
	registerGroup<StreamingTest>();
}
