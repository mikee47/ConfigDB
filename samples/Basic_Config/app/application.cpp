#include <SmingCore.h>
#include <LittleFS.h>
#include <IFS/Debug.h>
#include <basic-config.h>
#include <ConfigDB/Json/DataStream.h>
#include <Data/CStringArray.h>

#ifdef ENABLE_MALLOC_COUNT
#include <malloc_count.h>
#endif

extern void listProperties(ConfigDB::Database& db, Print& output);
extern void checkPerformance(BasicConfig& db);

namespace
{
IMPORT_FSTR(sampleConfig, PROJECT_DIR "/sample-config.json")

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
[[maybe_unused]] void readWriteValues(BasicConfig& db)
{
	Serial << endl << _F("** Read/Write Values **") << endl;

	{
		BasicConfig::Root::Security sec(db);
		sec.setApiSecured(true);
		sec.commit();
	}

	{
		BasicConfig::General general(db);
		general.setDeviceName(F("Test Device #") + os_random());
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

	{
		BasicConfig::General::Channels channels(db);
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
		item.commit();
		Serial << channels.getPath() << " = " << item << endl;
	}

	{
		BasicConfig::General::SupportedColorModels models(db);
		models.addItem(F("New Model #") + os_random());
		models.commit();
		Serial << models.getPath() << " = " << models << endl;
	}
}

/*
 * Test output from DataStream
 */
[[maybe_unused]] void stream(BasicConfig& db)
{
	Serial << endl << _F("** Stream **") << endl;

	ConfigDB::DataStream stream(db);
	Serial.copyFrom(&stream);
}

void printPoolData(const String& name, const ConfigDB::PoolData& data)
{
	Serial << "  " << String(name).pad(16) << ": " << data.usage() << " (*" << data.getItemSize() << ", "
		   << data.getCount() << " / " << data.getCapacity() << ')' << endl;
}

void printStringPool(ConfigDB::StringPool& pool, bool detailed)
{
	CStringArray csa(pool.getBuffer(), pool.getCount());
	auto n = csa.count();
	printPoolData(F("StringPool"), pool);

	if(!detailed) {
		return;
	}

	auto start = csa.c_str();
	unsigned i{0};
	for(auto s : csa) {
		String tag;
		tag += "    #";
		tag.concat(i, DEC, 2, ' ');
		tag += " [";
		tag += s - start;
		tag += ']';
		Serial << tag.pad(18) << ": \"" << s << '"' << endl;
		++i;
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
		Serial << F("  Root: ") << store->getTypeinfo().object.structSize << endl;
		printStringPool(store->stringPool, detailed);
		printArrayPool(store->arrayPool, detailed);

		size_t usage = store->stringPool.usage() + store->arrayPool.usage();
		Serial << "  Total usage = " << usage << endl;
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
	// db.setFormat(ConfigDB::Format::Pretty);

	readWriteValues(db);

	stream(db);

	listProperties(db, Serial);
	// checkPerformance(db);

	Serial << endl << endl;

	printStoreStats(db, false);
	printHeap();

	IFS::Debug::listDirectory(Serial, *IFS::getDefaultFileSystem(), db.getPath());

#ifdef ARCH_HOST
	System.restart();
#endif
}
