#include <SmingCore.h>
#include <LittleFS.h>
#include <IFS/Debug.h>
#include <basic-config.h>
#include <ConfigDB/Json/Reader.h>
#include <ConfigDB/Json/Writer.h>
#include <ConfigDB/Json/WriteStream.h>
#include <Data/CStringArray.h>
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
		if(auto update = sec.update()) {
			update.setApiSecured(true);
		}
	}

	{
		BasicConfig::General::OuterUpdater general(database);
		general.setDeviceName(F("Test Device #") + os_random());
		Serial << general.getPath() << ".deviceName = " << general.getDeviceName() << endl;
	}

	{
		BasicConfig::Color color(database);
		auto update = color.update();
		update.colortemp.setWw(12);
		Serial << color.colortemp.getPath() << ".WW = " << color.colortemp.getWw() << endl;

		auto& bri = color.brightness;
		update.brightness.setBlue(12);
		Serial << bri.getPath() << ".Blue = " << bri.getBlue() << endl;

		// OK, since we have an update in progress let's try another one asynchronously
		BasicConfig::Color::Brightness::Struct values{
			.red = 12,
			.green = 44,
			.blue = 8,
		};
		auto async =
			BasicConfig::Color::Brightness(database).update([values](BasicConfig::Color::Brightness::Updater upd) {
				Serial << "ASYNC UPDATE" << endl;
				upd.setRed(values.red);
				upd.setGreen(values.green);
				upd.setBlue(values.blue);
			});
		Serial << F("Async update ") << (async ? "completed" : "pending") << endl;
	}

	{
		BasicConfig::Events::OuterUpdater events(database);
		events.setColorMinintervalMs(1200);
	}

	{
		BasicConfig::General::Channels::OuterUpdater channels(database);
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

		Serial << channels.getPath() << " = " << item << endl;
	}

	{
		BasicConfig::General::SupportedColorModels::OuterUpdater models(database);
		models.addItem(F("New Model #") + os_random());
		models.insertItem(0, F("Inserted at #0"));
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
	Serial << toString(request.method) << " REQ" << endl;

	if(request.method == HTTP_POST) {
		if(!request.args) {
			// Empty body?
			debug_e("Where is my stream?");
			return;
		}
		auto stream = static_cast<ConfigDB::Json::WriteStream*>(request.args);
		auto status = stream->getStatus();
		response.sendString(F("Result: ") + toString(stream->getStatus()));
		delete stream;
		switch(status) {
		case JSON::Status::EndOfDocument:
			break;
		case JSON::Status::Cancelled:
			response.code = HTTP_STATUS_CONFLICT;
			break;
		default:
			response.code = HTTP_STATUS_BAD_REQUEST;
		}
		request.args = nullptr;
		return;
	}

	assert(!request.args);

	if(request.method != HTTP_GET) {
		return;
	}

	auto stream = ConfigDB::Json::reader.createStream(database);
	auto mimeType = stream->getMimeType();
	response.sendDataStream(stream.release(), mimeType);
}

/*
 * This parses incoming JSON data and updates the database
 */
size_t bodyToConfigParser(HttpRequest& request, const char* at, int length)
{
	if(request.method != HTTP_POST) {
		return 0;
	}

	if(length == PARSE_DATASTART) {
		assert(request.args == nullptr);
		auto stream = ConfigDB::Json::writer.createStream(database);
		request.args = stream.release();
		return 0;
	}

	auto stream = static_cast<Print*>(request.args);
	if(stream == nullptr) {
		debug_e("Invalid request argument");
		return 0;
	}

	if(length == PARSE_DATAEND || length < 0) {
		return 0;
	}

	return stream->write(at, length);
}

void startWebServer()
{
	server.listen(80);
	server.paths.setDefault(onFile);
	server.setBodyParser(MIME_JSON, bodyToConfigParser);

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

	printStoreStats(database, true);

	// Un-comment this line to test web client locking conflict behaviour
	// auto dirtyLock = new BasicConfig::Root::OuterUpdater(database);

	statTimer.initializeMs<5000>([]() {
		printHeap();
		IFS::Debug::listDirectory(Serial, *IFS::getDefaultFileSystem(), database.getPath());
	});
	statTimer.start();
}
