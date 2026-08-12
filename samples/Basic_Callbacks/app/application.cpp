#include <SmingCore.h>
#include <WebInfo.h>
#include <ConfigDB/Json/Format.h>
#include <ConfigDB/Network/HttpImportResource.h>
#include <Data/Format/Json.h>

#ifdef ENABLE_MALLOC_COUNT
#include <malloc_count.h>
#endif

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
#define WIFI_SSID "PleaseEnterSSID" // Put your SSID and password here
#define WIFI_PWD "PleaseEnterPass"
#endif

extern void listProperties(ConfigDB::Database& db, Print& output);

namespace
{
WebInfo database("test");
HttpServer server;
SimpleTimer statTimer;

IMPORT_FSTR(sampleData, PROJECT_DIR "/WebInfo.json")

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

	unsigned i = 0;
	for(unsigned id = 1; auto string = pool[id]; ++i) {
		String tag;
		tag += "    #";
		tag.concat(i, DEC, 2, ' ');
		tag += " [";
		tag += id;
		tag += ']';
		String s(string);
		Format::json.escape(s);
		Format::json.quote(s);
		Serial << tag.pad(18) << ": " << s << endl;
		id += string.getStorageSize();
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
	for(unsigned i = 0; i < db.typeinfo.storeCount; ++i) {
		auto store = db.openStore(i);
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
	Serial << toString(request.method) << " \"" << request.uri.getRelativePath() << '"' << endl;

	if(request.method != HTTP_GET) {
		response.code = HTTP_STATUS_BAD_REQUEST;
		return;
	}

	auto stream = database.createExportStream(ConfigDB::Json::format, request.uri.getRelativePath());
	response.sendDataStream(stream.release(), MIME_JSON);
}

void startWebServer()
{
	server.listen(80);
	server.paths.setDefault(onFile);
	server.paths.set(F("/update"), new ConfigDB::HttpImportResource(database, ConfigDB::Json::format));

	Serial.println("\r\n=== WEB SERVER STARTED ===");
	Serial.println(WifiStation.getIP());
	Serial.println("==============================\r\n");
}

void gotIP(IpAddress, IpAddress, IpAddress)
{
	startWebServer();
}

} // namespace

void init()
{
	Serial.begin(COM_SPEED_SERIAL);
	Serial.systemDebugOutput(true);

	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiAccessPoint.enable(false);

	WifiEvents.onStationGotIP(gotIP);

#ifdef ARCH_HOST
	fileSetFileSystem(&IFS::Host::getFileSystem());
#else
	spiffs_mount();
#endif

	/*
		Use static method to configure callbacks, it's more efficient.
		If an instance is already available then we can do this:

			WebInfo::Root root(database);
			root.onCommit([](auto root) {
				...
			});
	 */
	WebInfo::Root::onCommit(database, [](auto root) {
		root.app.setWebappVersion("1.2.3.4 : comment here just to affect string pool storage");

		Serial << _F("COMMIT CALLBACK!") << endl << root.app << endl;
		root.clearDirty();
	});

	FSTR::Stream source(sampleData);
	database.importFromStream(ConfigDB::Json::format, source);

	// ConfigDB::Json::format.setPretty(true);

	// database.exportToFile(ConfigDB::Json::format, F("out/database.json"));
	// database.importFromFile(ConfigDB::Json::format, F("out/database.json"));

	// stream(database);

	// listProperties(database, Serial);

	Serial << endl << endl;

	printStoreStats(database, true);

	// Un-comment this line to test web client locking conflict behaviour
	// auto dirtyLock = new BasicConfig::Root::OuterUpdater(database);

	statTimer.initializeMs<5000>([]() {
		printHeap();
		printStoreStats(database, true);
	});
	statTimer.start();
}
