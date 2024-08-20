#include <SmingCore.h>
#include <LittleFS.h>
#include <IFS/Debug.h>
#include <basic-config.h>
#include <ConfigDB/Json/Reader.h>
#include <Data/CStringArray.h>

#ifdef ENABLE_MALLOC_COUNT
#include <malloc_count.h>
#endif

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
#define WIFI_SSID "PleaseEnterSSID" // Put your SSID and password here
#define WIFI_PWD "PleaseEnterPass"
#endif

extern void listProperties(ConfigDB::Database& db, Print& output);
extern void checkPerformance(BasicConfig& db);

namespace
{
IMPORT_FSTR(sampleConfig, PROJECT_DIR "/sample-config.json")

BasicConfig database("test");
HttpServer server;
SimpleTimer statTimer;

[[maybe_unused]] void printHeap()
{
	Serial << _F("Heap statistics") << endl;
	Serial << _F("  Free bytes:  ") << system_get_free_heap_size() << endl;
#ifdef ENABLE_MALLOC_COUNT
	Serial << _F("  Used:        ") << MallocCount::getCurrent() << endl;
	Serial << _F("  Peak used:   ") << MallocCount::getPeak() << endl;
	Serial << _F("  Allocations: ") << MallocCount::getAllocCount() << endl;
	Serial << _F("  Total used:  ") << MallocCount::getTotal() << endl;
#endif
}

/*
 * Read and write some values
 */
[[maybe_unused]] void readWriteValues()
{
	Serial << endl << _F("** Read/Write Values **") << endl;

	{
		BasicConfig::Root::Security sec(database);
		sec.setApiSecured(true);
		sec.commit();
	}

	{
		BasicConfig::General general(database);
		general.setDeviceName(F("Test Device #") + os_random());
		Serial << general.getPath() << ".deviceName = " << general.getDeviceName() << endl;
		general.commit();
	}

	{
		BasicConfig::Color color(database);
		color.colortemp.setWw(12);
		Serial << color.colortemp.getPath() << ".WW = " << color.colortemp.getWw() << endl;

		BasicConfig::Color::Brightness bri(database);
		bri.setBlue(12);
		Serial << bri.getPath() << ".Blue = " << bri.getBlue() << endl;

		color.commit();
	}

	{
		BasicConfig::Events events(database);
		events.setColorMinintervalMs(1200);
		events.commit();
	}

	{
		BasicConfig::General::Channels channels(database);
		auto item = channels.addItem();
		item.setName(F("Channel #") + os_random());
		item.setPin(12);
		item.details.setCurrentLimit(400);
		item.notes.addItem(_F("This is a nice pin"));
		item.notes.addItem(_F("It is useful"));
		item.notes.addItem(SystemClock.getSystemTimeString());
		for(unsigned i = 0; i < 16; ++i) {
			item.values.addItem(os_random());
		}

		Serial << "old note = " << item.notes[0] << endl;
		item.notes[0] = F("Overwriting nice pin");
		Serial << "new note = " << item.notes[0] << endl;
		assert(String(item.notes[0]) == F("Overwriting nice pin"));

		item.notes.insertItem(0, F("Inserted at #0 on ") + SystemClock.getSystemTimeString());
		item.notes.insertItem(2, F("Inserted at #2"));

		item.commit();
		Serial << channels.getPath() << " = " << item << endl;
	}

	{
		BasicConfig::General::SupportedColorModels models(database);
		models.addItem(F("New Model #") + os_random());
		models.insertItem(0, F("Inserted at #0"));
		models.commit();
		Serial << models.getPath() << " = " << models << endl;
	}
}

/*
 * Test output from ReadStream
 */
[[maybe_unused]] void stream(BasicConfig& db)
{
	Serial << endl << _F("** Stream **") << endl;

	auto stream = ConfigDB::Json::reader.createStream(database);
	Serial.copyFrom(stream.get());
}

void printPoolData(const String& name, const ConfigDB::PoolData& data)
{
	Serial << "  " << String(name).pad(16) << ": " << data.usage() << " (*" << data.getItemSize() << ", "
		   << data.getCount() << " / " << data.getCapacity() << ')' << endl;
}

void printStringPool(const ConfigDB::StringPool& pool, bool detailed)
{
	printPoolData(F("StringPool"), pool);

	if(!detailed) {
		return;
	}

	auto start = pool.getBuffer();
	auto end = start + pool.getCount();
	unsigned i = 0;
	for(auto s = start; s < end; ++i, s += strlen(s) + 1) {
		String tag;
		tag += "    #";
		tag.concat(i, DEC, 2, ' ');
		tag += " [";
		tag += s - start;
		tag += ']';
		Serial << tag.pad(18) << ": \"" << s << '"' << endl;
	}
}

void printArrayPool(const ConfigDB::ArrayPool& pool, bool detailed)
{
	auto n = pool.getCount();
	printPoolData(F("ArrayPool"), pool);
	//  << pool.usage() << " (" << n << " / " << pool.getCapacity() << ')' << endl;
	size_t used{0};
	size_t capacity{0};
	for(unsigned i = 1; i <= n; ++i) {
		auto& data = pool[i];
		if(detailed) {
			String tag;
			tag += "  [";
			tag += i;
			tag += ']';
			printPoolData(tag, data);
		}
		used += data.getItemSize() * data.getCount();
		capacity += data.getItemSize() * data.getCapacity();
	}
	Serial << "    Used:     " << used << endl;
	Serial << "    Capacity: " << capacity << endl;
}

void printStoreStats(ConfigDB::Database& db, bool detailed)
{
	for(unsigned i = 0; auto store = db.getStore(i); ++i) {
		Serial << F("Store '") << store->getName() << "':" << endl;
		Serial << F("  Root: ") << store->typeinfo().structSize << endl;
		printStringPool(store->getStringPool(), detailed);
		printArrayPool(store->getArrayPool(), detailed);

		size_t usage = store->getStringPool().usage() + store->getArrayPool().usage();
		Serial << "  Total usage = " << usage << endl;
	}
}

void onFile(HttpRequest& request, HttpResponse& response)
{
	auto stream = ConfigDB::Json::reader.createStream(database);
	auto mimeType = stream->getMimeType();
	response.sendDataStream(stream.release(), mimeType);
}

void startWebServer()
{
	server.listen(80);
	server.paths.setDefault(onFile);

	Serial.println("\r\n=== WEB SERVER STARTED ===");
	Serial.println(WifiStation.getIP());
	Serial.println("==============================\r\n");
}

void gotIP(IpAddress ip, IpAddress netmask, IpAddress gateway)
{
	startWebServer();
}

} // namespace

#include <ConfigDB/Json/Reader.h>

void init()
{
	Serial.begin(COM_SPEED_SERIAL);
	Serial.systemDebugOutput(true);

#ifdef ARCH_HOST
	fileSetFileSystem(&IFS::Host::getFileSystem());
#else
	lfs_mount();
#endif

	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiAccessPoint.enable(false);

	WifiEvents.onStationGotIP(gotIP);

	createDirectory("test");
	// ConfigDB::Json::reader.setFormat(ConfigDB::Json::Format::Pretty);

	readWriteValues();

	// ConfigDB::Json::reader.saveToFile(db, F("test/database.json"));
	// ConfigDB::Json::writer.loadFromFile(db, F("test/database.json"));

	// stream(database);

	// listProperties(database, Serial);
	// checkPerformance(database);

	Serial << endl << endl;

	printStoreStats(database, false);

	statTimer.initializeMs<5000>([]() {
		printHeap();
		IFS::Debug::listDirectory(Serial, *IFS::getDefaultFileSystem(), database.getPath());
	});
	statTimer.start();
}
