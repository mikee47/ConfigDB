#include <SmingCore.h>
#include <LittleFS.h>

// Store JSON in easy-to-read format
#define JSON_FORMAT_DEFAULT Json::Pretty

#include <basic-config.h>
#include <ConfigDB/DataStream.h>

namespace
{
IMPORT_FSTR(sampleConfig, PROJECT_DIR "/sample-config.json")

/*
 * Analyse ArduinoJson memory usage for the configuration as a whole,
 * and if each top-level object is handled as a separate document.
 */
[[maybe_unused]] void checkConfig()
{
	DynamicJsonDocument doc(4096);
	assert(Json::deserialize(doc, sampleConfig));
	Serial << "MemoryUsage " << doc.memoryUsage() << endl;

	size_t totalMem{0};
	for(auto elem : doc.as<JsonObject>()) {
		String content = Json::serialize(elem.value());
		DynamicJsonDocument doc2(4096);
		assert(Json::deserialize(doc2, content));
		Serial << "Element " << elem.key().c_str() << " requires " << doc2.memoryUsage() << " bytes" << endl;
		totalMem += doc2.memoryUsage();
	}
	Serial << "Total memoryUsage " << totalMem << endl;
}

/*
 * Read and write some values
 */
void readWriteValues(BasicConfig& db)
{
	Serial << endl << _F("** Read/Write Values **") << endl;

	{
		BasicConfig::Security sec(db);
		sec.setApiSecured(true);
		sec.commit();
	}

	{
		BasicConfig::General general(db);
		general.setDeviceName("Test Device");
		Serial << general.getPath() << ".deviceName = " << general.getDeviceName() << endl;
		general.commit();
	}

	{
		BasicConfig::Color color(db);
		color.colortemp.setWw(12);
		Serial << color.colortemp.getPath() << ".WW = " << color.colortemp.getWw() << endl;

		BasicConfig::Color::Brightness bri(db);
		bri.setBlue(12);
		Serial << bri.getPath() << ".Blue = " << bri.getBlue() << endl;

		color.commit();
	}

	{
		BasicConfig::Events events(db);
		events.setColorMinintervalMs(1200);
		events.commit();
	}
}

/*
 * Prints out database content.
 * Should be identical to stream output.
 */
void inspect(BasicConfig& db)
{
	Serial << endl << _F("** Inspect **") << endl;

	Serial << '{' << endl;
	for(unsigned i = 0; auto store = db.getStore(i); ++i) {
		if(i > 0) {
			Serial << ',' << endl;
		}
		if(auto& name = store->getName()) {
			Serial << '"' << name << "\":";
		}
		for(unsigned j = 0; auto obj = store->getObject(j); ++j) {
			if(j > 0) {
				Serial << ',' << endl;
			}
			if(auto& name = obj->getName()) {
				Serial << '"' << name << "\":";
			}
			Serial << *obj;
		}
	}
	Serial << endl << '}' << endl;
}

/*
 * Test output from DataStream
 */
void stream(BasicConfig& db)
{
	Serial << endl << _F("** Stream **") << endl;

	ConfigDB::DataStream stream(db);
	Serial.copyFrom(&stream);
}

} // namespace

void init()
{
	Serial.begin(COM_SPEED_SERIAL);
	Serial.systemDebugOutput(true);

#ifdef ARCH_HOST
	fileSetFileSystem(&IFS::Host::getFileSystem());
#else
	lfs_mount();
#endif

	createDirectory("test");
	BasicConfig db("test");

	// checkConfig();
	readWriteValues(db);
	inspect(db);
	stream(db);

	Serial << endl << endl;

#ifdef ARCH_HOST
	System.restart();
#endif
}
