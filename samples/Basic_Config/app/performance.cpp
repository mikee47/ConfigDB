#include <basic-config.h>
#include <Services/Profiling/MinMaxTimes.h>
#include <Data/Stream/MemoryDataStream.h>
#include <HardwareSerial.h>

namespace
{
using Callback = Delegate<void(BasicConfig& db, unsigned round)>;

void __noinline profile(BasicConfig& db, const String& title, Callback callback)
{
	const int rounds = 64;

	Profiling::MicroTimes times(title);
	for(int i = 0; i < rounds; i++) {
		times.start();
		callback(db, i);
		times.update();
	}
	Serial << times << endl;
}

} // namespace

void checkPerformance(BasicConfig& db)
{
	Serial << endl << _F("** Performance **") << endl;

	const int rounds = 64;

	Serial << _F("Evaluating load times ...") << endl;

	// Load same cache multiple times
	profile(db, F("Verify load caching"), [](BasicConfig& db, unsigned) { db.openStore(1); });

	// Load different stores in sequence to bypass caching
	profile(db, F("Load all stores"),
			[](BasicConfig& db, unsigned i) { auto store = db.openStore(i % db.typeinfo.storeCount); });

	/*
	 * Open Color store
	 *
	 * Store open/load is expensive. Open it once here.
	 * TODO: Database could cache the last used store, perhaps only closing it
	 *  	 when another is opened or a brief timeout elapses.
	 */
	BasicConfig::Color color(db);

	Serial << _F("Evaluating setValue / commit ...") << endl;

	profile(db, F("Set Value + commit"), [](BasicConfig& db, unsigned value) {
		BasicConfig::Color::Brightness::OuterUpdater bri(db);
		bri.setRed(value);
		bri.setGreen(value);
		bri.setBlue(value);
		bri.setWw(value);
		bri.setCw(value);
	});

	Serial << _F("Evaluating getValue ...") << endl;

	profile(db, F("getValue [simple]"), [](BasicConfig& db, unsigned) {
		BasicConfig::Color::Brightness bri(db);
		bri.getRed();
		bri.getGreen();
		bri.getBlue();
		bri.getCw();
		bri.getWw();
	});

	profile(db, F("getValue [build string]"), [](BasicConfig& db, unsigned value) {
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
	});

	profile(db, F("getValue [Print]"), [](BasicConfig& db, unsigned value) {
		MemoryDataStream stream;
		BasicConfig::Color::Brightness bri(db);
		stream << _F("read:") << value << _F("{r:") << bri.getRed() << _F(",g:") << bri.getGreen() << _F(",b:")
			   << bri.getBlue() << _F(",cw:") << bri.getCw() << _F(",ww:") << bri.getWw() << _F("}") << endl;
	});
}
