#include <ConfigDB.h>
#include <Print.h>

namespace
{
void printItem(Print& output, const String& tag, unsigned indent, const String& type, const String& name,
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
	output << s << value << endl;
}

void printObject(Print& output, const String& tag, unsigned indent, ConfigDB::Object& obj)
{
	printItem(output, tag, indent, toString(obj.getTypeinfo().type), obj.getName());
	auto n = obj.getPropertyCount();
	for(unsigned i = 0; i < n; ++i) {
		auto prop = obj.getProperty(i);
		String value;
		value += toString(prop.getInfo()->type);
		value += " = ";
		value += prop.getJsonValue();
		printItem(output, tag + '.' + i, indent + 1, F("Property"), prop.getInfo()->name, value);
	}
	n = obj.getObjectCount();
	for(unsigned i = 0; i < n; ++i) {
		printObject(output, tag + '.' + i, indent + 1, *obj.getObject(i));
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
	for(unsigned i = 0; auto store = db.getStore(i); ++i) {
		String tag(i);
		printItem(output, tag, 1, F("Store"), store->getName());
		printObject(output, tag + ".0", 2, *store->getObject());
	}
}
