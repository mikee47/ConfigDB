#include <basic-config.h>
#include <Services/Profiling/MinMaxTimes.h>
#include <Data/Stream/MemoryDataStream.h>

namespace
{
void __noinline testSetValue(BasicConfig& db, int value, bool commit)
{
	BasicConfig::Root::Color::Brightness bri(db);
	bri.setRed(value);
	bri.setGreen(value);
	bri.setBlue(value);
	bri.setWw(value);
	bri.setCw(value);
	if(commit) {
		bri.commit();
	}
}

void __noinline testGetValueSimple(BasicConfig& db)
{
	BasicConfig::Root::Color::Brightness bri(db);
	bri.getRed();
	bri.getGreen();
	bri.getBlue();
	bri.getCw();
	bri.getWw();
}

String __noinline testGetValueBuildString(BasicConfig& db, int value)
{
	BasicConfig::Root::Color::Brightness bri(db);
	String s;
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
	BasicConfig::Root::Color::Brightness bri(db);
	p << _F("read:") << value << _F("{r:") << bri.getRed() << _F(",g:") << bri.getGreen() << _F(",b:") << bri.getBlue()
	  << _F(",cw:") << bri.getCw() << _F(",ww:") << bri.getWw() << _F("}") << endl;
}

} // namespace

void checkPerformance(BasicConfig& db)
{
	Serial << endl << _F("** Performance **") << endl;

	const int rounds = 64;

	/*
	 * Open Color store
	 *
	 * Store open/load is expensive. Open it once here.
	 * TODO: Database could cache the last used store, perhaps only closing it
	 *  	 when another is opened or a brief timeout elapses.
	 */
	BasicConfig::Root::Color color(db);

	Serial << _F("Evaluating setValue / commit ...") << endl;
	{
		Profiling::MicroTimes times(F("Set Value only"));
		for(int i = 0; i < rounds; i++) {
			times.start();
			testSetValue(db, i, false);
			times.update();
		}
		Serial << times << endl;
	}

	{
		Profiling::MicroTimes times(F("Set Value + commit"));
		for(int i = 0; i < rounds; i++) {
			times.start();
			testSetValue(db, i, true);
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
		Profiling::MicroTimes times(F("getValue [Print]"));
		for(int i = 0; i < rounds; i++) {
			times.start();
			MemoryDataStream stream;
			testGetValuePrint(db, i, stream);
			times.update();
		}
		Serial << times << endl;
	}
}
