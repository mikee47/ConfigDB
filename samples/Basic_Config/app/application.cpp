#include <SmingCore.h>
#include <LittleFS.h>

#include <basic-config.h>
#include <ConfigDB/DataStream.h>

extern void checkPerformance(BasicConfig& db);

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
		BasicConfig::Root::Security sec(db);
		sec.setApiSecured(true);
		sec.commit();
	}

	{
		BasicConfig::Root::General general(db);
		general.setDeviceName("Test Device");
		Serial << general.getPath() << ".deviceName = " << general.getDeviceName() << endl;
		general.commit();
	}

	{
		BasicConfig::Root::Color color(db);
		color.colortemp.setWw(12);
		Serial << color.colortemp.getPath() << ".WW = " << color.colortemp.getWw() << endl;

		BasicConfig::Color::Brightness bri(db);
		bri.setBlue(12);
		Serial << bri.getPath() << ".Blue = " << bri.getBlue() << endl;

		color.commit();
	}

	{
		BasicConfig::Root::Events events(db);
		events.setColorMinintervalMs(1200);
		events.commit();
	}

	{
		BasicConfig::General::Channels channels(db);
		auto item = channels.addItem();
		item.setName("Channel Name");
		item.setPin(12);
		item.commit();
		// item = channels.getItem(0);
		Serial << channels.getPath() << " = " << item << endl;
	}

	{
		BasicConfig::General::SupportedColorModels models(db);
		models.addItem("New Model");
		models.commit();
		Serial << models.getPath() << " = " << models << endl;
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

void printItem(const String& tag, unsigned indent, const String& type, const String& name,
			   const String& value = nullptr)
{
	String s;
	s.pad(indent, '\t');
	s += '#';
	s += tag;
	s += ' ';
	s += type;
	s += " \"";
	s += name;
	s += '"';
	if(value) {
		s += ": ";
	}
	Serial << s << value << endl;
}

void printObject(const String& tag, unsigned indent, ConfigDB::Object& obj)
{
	printItem(tag, indent, F("Object"), obj.getName());
	for(unsigned i = 0; auto prop = obj.getProperty(i); ++i) {
		String value;
		value += toString(prop.getType());
		value += " = ";
		value += prop.getJsonValue();
		printItem(tag + '.' + i, indent + 1, F("Property"), prop.getName(), value);
	}
	for(unsigned j = 0; auto child = obj.getObject(j); ++j) {
		printObject(tag + '.' + j, indent + 1, *child);
	}
}

/*
 * Inspect database objects and properties recursively
 */
void listProperties(BasicConfig& db)
{
	Serial << endl << _F("** Inspect Properties **") << endl;

	Serial << _F("Database \"") << db.getName() << '"' << endl;
	for(unsigned i = 0; auto store = db.getStore(i); ++i) {
		String tag(i);
		printItem(tag, 1, F("Store"), store->getName());
		for(unsigned j = 0; auto obj = store->getObject(j); ++j) {
			printObject(tag + '.' + j, 2, *obj);
		}
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

	createDirectory("test");
	BasicConfig db("test");
	db.setFormat(ConfigDB::Format::Pretty);

	// checkConfig();
	readWriteValues(db);
	inspect(db);
	stream(db);
	listProperties(db);
	// checkPerformance(db);

	Serial << endl << endl;

#ifdef ARCH_HOST
	System.restart();
#endif
}
