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
	for(unsigned i = 0; auto prop = obj.getProperty(i); ++i) {
		String value;
		value += toString(prop.info.type);
		value += " = ";
		value += prop.getJsonValue();
		printItem(output, tag + '.' + i, indent + 1, F("Property"), prop.info.name, value);
	}
	for(unsigned j = 0; auto child = obj.getObject(j); ++j) {
		printObject(output, tag + '.' + j, indent + 1, *child);
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
