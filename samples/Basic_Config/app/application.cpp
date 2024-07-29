#include <SmingCore.h>
#include <LittleFS.h>

// Store JSON in easy-to-read format
#define JSON_FORMAT_DEFAULT Json::Pretty

#include <basic-config.h>

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

void testStore()
{
	BasicConfig db("test");

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

	// checkConfig();
	testStore();

	System.restart();
}
