/*
 * Streaming.cpp
 */

#include <ConfigDBTest.h>
#include <Data/Stream/MemoryDataStream.h>
#include <ConfigDB/Json/Format.h>

/*
 * Array selector test data is generated using a python script.
 * C strings are unreadable as they require escapes.
 * So put the JSON data into an imported file and reference by position and length.
 * This requires the python script to generate two files.
 */
// Overwrite array
struct ArrayTestCase {
	struct Pos {
		uint16_t offset;
		uint16_t length;

		String get(const FSTR::String& data) const
		{
			char buffer[length];
			data.read(offset, buffer, length);
			return String(buffer, length);
		}
	};

	Pos expr;
	Pos result;
};

#include <array-test.h>

namespace json
{
// Embedding JSON in C strings is virtual unreadable, just import it
IMPORT_FSTR_LOCAL(update1, PROJECT_DIR "/resource/update1.json")
IMPORT_FSTR_LOCAL(root1, PROJECT_DIR "/resource/root1.json")
IMPORT_FSTR_LOCAL(array_test_default, PROJECT_DIR "/resource/array_test_default.json")
} // namespace json

class StreamingTest : public TestGroup
{
public:
	StreamingTest() : TestGroup(_F("Streaming"))
	{
		resetDatabase();
	}

	void execute() override
	{
		// general();
		arrays();
	}

	void general()
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
	}

	template <typename T> bool importObject(T& object, const String& data)
	{
		MemoryDataStream mem;
		mem << data;
		return object.importFromStream(ConfigDB::Json::format, mem);
	}

	template <typename T> String exportObject(T& object)
	{
		MemoryDataStream mem;
		object.exportToStream(ConfigDB::Json::format, mem);
		String s;
		mem.moveString(s);
		return s;
	}

	void arrays()
	{
		TEST_CASE("Indexed int array update")
		{
			for(auto test : int_array_test_cases) {
				String expr = test.expr.get(arrayTestData);
				String result = test.result.get(arrayTestData);
				bool isValid = (result[0] == '[');
				Serial << expr << " -> ";
				TestConfig::Root::OuterUpdater root(database);
				CHECK(root);
				CHECK(importObject(root, json::array_test_default));
				CHECK_EQ(importObject(root, expr), isValid);
				if(isValid) {
					String s = exportObject(root.intArray);
					Serial << s;
					CHECK_EQ(s, result);
				}
				Serial << " - " << result << endl;
			}
		}

		TEST_CASE("Indexed object array update")
		{
			for(auto test : object_array_test_cases) {
				String expr = test.expr.get(arrayTestData);
				String result = test.result.get(arrayTestData);
				bool isValid = (result[0] == '[');
				Serial << expr << " -> ";
				TestConfig::Root::OuterUpdater root(database);
				CHECK(root);
				CHECK(importObject(root, json::array_test_default));
				CHECK_EQ(importObject(root, expr), isValid);
				if(isValid) {
					String s = exportObject(root.objectArray);
					Serial << s;
					CHECK_EQ(s, result);
				}
				Serial << " - " << result << endl;
			}
		}
	}
};

void REGISTER_TEST(Streaming)
{
	registerGroup<StreamingTest>();
}
