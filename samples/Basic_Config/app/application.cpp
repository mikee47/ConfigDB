#include <SmingCore.h>
#include <LittleFS.h>

#include <basic-config.h>
#include <ConfigDB/Json/DataStream.h>

extern void listProperties(ConfigDB::Database& db, Print& output);
extern void checkPerformance(BasicConfig& db);

namespace
{
IMPORT_FSTR(sampleConfig, PROJECT_DIR "/sample-config.json")

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

	{
		BasicConfig::General::Channels channels(db);
		auto item = channels.addItem();
		item.setName("Channel Name");
		item.setPin(12);
		item.details.setCurrentLimit(400);
		item.notes.addItem(_F("This is a nice pin"));
		item.notes.addItem(_F("It is useful"));
		for(unsigned i = 0; i < 16; ++i) {
			item.values.addItem(os_random());
		}
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
 * Test output from DataStream
 */
[[maybe_unused]] void stream(BasicConfig& db)
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

#if 0
	FSTR::Stream stream(sampleConfig);
	parseJson(stream);
#else

#ifdef ARCH_HOST
	fileSetFileSystem(&IFS::Host::getFileSystem());
#else
	lfs_mount();
#endif

	createDirectory("test");
	BasicConfig db("test");
	db.setFormat(ConfigDB::Format::Pretty);

	readWriteValues(db);
	stream(db);
	// listProperties(db, Serial);
	// checkPerformance(db);

	Serial << endl << endl;
#endif

#ifdef ARCH_HOST
	System.restart();
#endif
}
