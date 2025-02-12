/*
 * Streaming.cpp
 */

#include <ConfigDBTest.h>
#include <Data/CStringArray.h>

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

IMPORT_FSTR_LOCAL(arrayTestData, PROJECT_DIR "/resource/array-test.json")
#include <array-test.h>

namespace json
{
// Embedding JSON in C strings is virtual unreadable, just import it
IMPORT_FSTR_LOCAL(update1, PROJECT_DIR "/resource/update1.json")
IMPORT_FSTR_LOCAL(root1, PROJECT_DIR "/resource/root1.json")
IMPORT_FSTR_LOCAL(database1, PROJECT_DIR "/resource/database1.json")
IMPORT_FSTR_LOCAL(array_test_default, PROJECT_DIR "/resource/array_test_default.json")
IMPORT_FSTR_LOCAL(async_update, PROJECT_DIR "/resource/async-update.json")
IMPORT_FSTR_LOCAL(async_update_result, PROJECT_DIR "/resource/async-update-result.json")
} // namespace json

namespace
{
String getContent(IDataSourceStream& stream)
{
	MemoryDataStream mem;
	mem.copyFrom(&stream);
	String content;
	mem.moveString(content);
	return content;
}

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
		general();
		importExport();
		arrays();
		importAsync();
	}

	void general()
	{
		// Verify initial value
		TestConfig::Root root(database);
		CHECK(!root.getSimpleBool());

		// Check streaming into read-only object fails, and data is not changed
		TEST_CASE("Streaming import, read-only")
		{
			REQUIRE(!importObject(root, json::update1));
			REQUIRE(!root.getSimpleBool());
			REQUIRE_EQ(root.getSimpleInt(), -1);
		}

		// Change value
		TEST_CASE("Streaming import, updater")
		{
			if(auto updater = root.update()) {
				REQUIRE(importObject(updater, json::update1));
				REQUIRE_EQ(root.getSimpleBool(), true);
				REQUIRE_EQ(root.getSimpleInt(), 100);
			} else {
				TEST_ASSERT(false);
			}

			REQUIRE_EQ(exportObject(root), json::root1);
			REQUIRE_EQ(exportObject(database), json::database1);
		}
	}

	void importExport()
	{
		/* Streaming objects */
		resetDatabase();

		TEST_CASE("Streaming object import")
		{
			TestConfig::Root root(database);
			if(auto updater = root.update()) {
				FlashMemoryStream source(json::update1);
				auto stream = updater.createImportStream(ConfigDB::Json::format);
				REQUIRE_EQ(stream->copyFrom(&source), json::update1.length());
				REQUIRE_EQ(root.getSimpleBool(), true);
			} else {
				TEST_ASSERT(false);
			}
		}

		TEST_CASE("Streaming object export")
		{
			TestConfig::Root root(database);
			MemoryDataStream mem;
			auto stream = root.createExportStream(ConfigDB::Json::format);
			String content = getContent(*stream);
			REQUIRE_EQ(content, json::root1);
		}

		TEST_CASE("Streaming export options (intarray)")
		{
			TestConfig::Root::IntArray intarray(database);
			auto stream = intarray.createExportStream(ConfigDB::Json::format);
			REQUIRE_EQ(getContent(*stream), F("[13,28,39,40]"));
			stream = intarray.createExportStream(ConfigDB::Json::format, {.useName = true});
			REQUIRE_EQ(getContent(*stream), F("\"int_array\":[13,28,39,40]"));
			stream = intarray.createExportStream(ConfigDB::Json::format, {.asObject = true});
			REQUIRE_EQ(getContent(*stream), F("{\"int_array\":[13,28,39,40]}"));
		}

		TEST_CASE("Streaming export options (root)")
		{
			// An un-named root object should produce same output (i.e. options are ignored)
			String expectedContent =
				F("{\"int_array\":[13,28,39,40],\"string_array\":[\"a\",\"b\",\"c\"],\"object_array\":[],\"color\":"
				  "\"red\",\"simple-bool\":true,\"simple-string\":\"donkey\",\"simple-int\":100,\"simple-float\":3."
				  "1415927}");
			TestConfig::Root root(database);
			auto stream = root.createExportStream(ConfigDB::Json::format);
			REQUIRE_EQ(getContent(*stream), expectedContent);
			stream = root.createExportStream(ConfigDB::Json::format, {.useName = true});
			REQUIRE_EQ(getContent(*stream), expectedContent);
			stream = root.createExportStream(ConfigDB::Json::format, {.asObject = true});
			REQUIRE_EQ(getContent(*stream), expectedContent);
		}

		TEST_CASE("Streaming export options (database)")
		{
			auto stream = database.createExportStream(ConfigDB::Json::format);
			REQUIRE_EQ(getContent(*stream), json::database1);

			stream = database.createExportStream(ConfigDB::Json::format, nullptr, {.useName = true});
			String withName = F("\"test-config\":") + json::database1;
			REQUIRE_EQ(getContent(*stream), withName);

			stream = database.createExportStream(ConfigDB::Json::format, nullptr, {.asObject = true});
			REQUIRE_EQ(getContent(*stream), '{' + withName + '}');
		}

		TEST_CASE("Streaming database import")
		{
			resetDatabase();
			FlashMemoryStream source(json::update1);
			auto stream = database.createImportStream(ConfigDB::Json::format);
			REQUIRE_EQ(stream->copyFrom(&source), json::update1.length());
			stream.reset();
			TestConfig::Root root(database);
			REQUIRE_EQ(root.getSimpleBool(), true);
		}
	}

	void importAsync()
	{
		TEST_CASE("Streaming object import (async)")
		{
			importStream = database.createImportStream(ConfigDB::Json::format);
			importSource = std::make_unique<FlashMemoryStream>(json::async_update);

			importTimer.initializeMs<500>([this]() { importAsyncPart(); });
			importTimer.start();

			pending();
		}
	}

	void importAsyncPart()
	{
		if(!importStream) {
			TestConfig::Root root(database);
			Serial << root << endl;
			CHECK_EQ(exportObject(root), json::async_update_result);
			complete();
			return;
		}

		uint8_t buffer[55];
		auto len = importSource->readBytes(buffer, sizeof(buffer));
		m_printHex("importSource", buffer, len, 0);
		importStream->write(buffer, len);
		if(len < sizeof(buffer)) {
			importStream.reset();
		}
	}

	void arrays()
	{
		TEST_CASE("Indexed int array update")
		{
			arrayTests(int_array_test_cases, false);
		}

		TEST_CASE("Indexed object array update")
		{
			arrayTests(object_array_test_cases, true);
		}

		TEST_CASE("Indexed object array update against database")
		{
			for(auto test : object_array_test_cases) {
				String expr = test.expr.get(arrayTestData);
				Serial << "CASE: " << expr << endl;
				String result = test.result.get(arrayTestData);
				bool isValid = (result[0] == '[');
				Serial << expr << " -> ";
				CHECK(importObject(database, json::array_test_default));
				CHECK_EQ(importObject(database, expr), isValid);
				if(isValid) {
					TestConfig::Root root(database);
					String s = exportObject(root.objectArray);
					Serial << s;
					REQUIRE_EQ(s, result);
				}
				Serial << " - " << result << endl;
			}
		}
	}

	void arrayTests(const FSTR::Array<ArrayTestCase>& testCases, bool isObject)
	{
		for(auto test : testCases) {
			String expr = test.expr.get(arrayTestData);
			String result = test.result.get(arrayTestData);
			bool isValid = (result[0] == '[');
			Serial << expr << " -> ";
			TestConfig::Root::OuterUpdater root(database);
			CHECK(root);
			CHECK(importObject(root, json::array_test_default));
			CHECK_EQ(importObject(root, expr), isValid);
			if(isValid) {
				String s = isObject ? exportObject(root.objectArray) : exportObject(root.intArray);
				Serial << s;
				CHECK_EQ(s, result);
			}
			Serial << " - " << result << endl;
			root.clearDirty();
		}
	}

private:
	std::unique_ptr<ConfigDB::ImportStream> importStream;
	std::unique_ptr<IDataSourceStream> importSource;
	Timer importTimer;
};

void REGISTER_TEST(Streaming)
{
	registerGroup<StreamingTest>();
}
