#include <ConfigDB.h>
#include <Print.h>

namespace
{
void printItem(Print& output, const String& type, const String& name, const String& value = nullptr)
{
	String s;
	s += name;
	s += ": ";
	s += type;
	if(value) {
		s += " = ";
	}
	output << s << value << endl;
}

void printObject(Print& output, const ConfigDB::Object& obj)
{
	printItem(output, toString(obj.typeinfo().type), obj.getPath());
	auto n = obj.getPropertyCount();
	for(unsigned i = 0; i < n; ++i) {
		auto prop = obj.getProperty(i);
		String value = prop.getJsonValue();
		String name = prop.info().name;
		if(!name.length()) {
			name = String(i);
		}
		printItem(output, toString(prop.info().type), obj.getPath() + "/" + name, value);
	}
	n = obj.getObjectCount();
	for(unsigned i = 0; i < n; ++i) {
		auto child = obj.getObject(i);
		printObject(output, child);
	}
}

} // namespace

/*
 * Inspect database objects and properties recursively
 */
void listProperties(ConfigDB::Database& db, Print& output)
{
	output << endl << _F("** Inspect Properties **") << endl;

	output << _F("Database \"") << db.getName() << '"' << endl;
	for(unsigned i = 0; i < db.typeinfo.storeCount; ++i) {
		auto store = db.openStore(i);
		printObject(output, *store);
	}
}
