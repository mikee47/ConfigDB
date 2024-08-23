#include <basic-config.h>
#include <Services/Profiling/MinMaxTimes.h>
#include <Data/Stream/MemoryDataStream.h>
#include <HardwareSerial.h>

namespace
{
void __noinline testSetValue(BasicConfig& db, int value)
{
	BasicConfig::Color::Brightness::Updater bri(db);
	bri.setRed(value);
	bri.setGreen(value);
	bri.setBlue(value);
	bri.setWw(value);
	bri.setCw(value);
}

void __noinline testGetValueSimple(BasicConfig& db)
{
	BasicConfig::Color::Brightness bri(db);
	bri.getRed();
	bri.getGreen();
	bri.getBlue();
	bri.getCw();
	bri.getWw();
}

String __noinline testGetValueBuildString(BasicConfig& db, int value)
{
	BasicConfig::Color::Brightness bri(db);
	String s;
	s.reserve(128);
	s += _F("read:");
	s += value;
	s += _F("{r:");
	s += bri.getRed();
	s += _F(",g:");
	s += bri.getGreen();
	s += _F(",b:");
	s += bri.getBlue();
	s += _F(",cw:");
	s += bri.getCw();
	s += _F(",ww:");
	s += bri.getWw();
	s += _F("}");
	return s;
}

void __noinline testGetValuePrint(BasicConfig& db, int value, Print& p)
{
	BasicConfig::Color::Brightness bri(db);
	p << _F("read:") << value << _F("{r:") << bri.getRed() << _F(",g:") << bri.getGreen() << _F(",b:") << bri.getBlue()
	  << _F(",cw:") << bri.getCw() << _F(",ww:") << bri.getWw() << _F("}") << endl;
}

} // namespace

void checkPerformance(BasicConfig& db)
{
	Serial << endl << _F("** Performance **") << endl;

	const int rounds = 64;

	Serial << _F("Evaluating load times ...") << endl;
	{
		// Load same cache multiple times
		Profiling::MicroTimes times(F("Verify load caching"));
		for(int i = 0; i < rounds; i++) {
			times.start();
			db.openStore(1);
			times.update();
		}
		Serial << times << endl;
	}

	{
		// Load different stores in sequence to bypass caching
		Profiling::MicroTimes times(F("Load all stores"));
		for(int i = 0; i < rounds; i++) {
			times.start();
			auto store = db.openStore(i % db.typeinfo.storeCount);
			times.update();
		}
		Serial << times << endl;
	}

	/*
	 * Open Color store
	 *
	 * Store open/load is expensive. Open it once here.
	 * TODO: Database could cache the last used store, perhaps only closing it
	 *  	 when another is opened or a brief timeout elapses.
	 */
	BasicConfig::Color color(db);

	Serial << _F("Evaluating setValue / commit ...") << endl;
	{
		Profiling::MicroTimes times(F("Set Value + commit"));
		for(int i = 0; i < rounds; i++) {
			times.start();
			testSetValue(db, i);
			times.update();
		}
		Serial << times << endl;
	}

	Serial << _F("Evaluating getValue ...") << endl;

	{
		Profiling::MicroTimes times(F("getValue [simple]"));
		for(int i = 0; i < rounds; i++) {
			times.start();
			testGetValueSimple(db);
			times.update();
		}
		Serial << times << endl;
	}

	{
		Profiling::MicroTimes times(F("getValue [build string]"));
		for(int i = 0; i < rounds; i++) {
			times.start();
			String s = testGetValueBuildString(db, i);
			times.update();
		}
		Serial << times << endl;
	}

	{
		MemoryDataStream stream;
		Profiling::MicroTimes times(F("getValue [Print]"));
		for(int i = 0; i < rounds; i++) {
			stream.clear();
			times.start();
			testGetValuePrint(db, i, stream);
			times.update();
		}
		Serial << times << endl;
	}
}
