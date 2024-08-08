#include <basic-config.h>
#include <JSON/StreamingParser.h>
#include <Data/Format/Standard.h>
#include <HardwareSerial.h>

namespace
{
class ConfigListener : public JSON::Listener
{
public:
	using Element = JSON::Element;

	ConfigListener(ConfigDB::Database& db, Print& output) : db(db), output(output)
	{
	}

	/*
	 * Top level key can match against a store name or an entry in the root object.
	 * Return the matching object, or nullptr.
	 */
	const ConfigDB::ObjectInfo* rootSearch(const String& key)
	{
		// Find store
		auto& dbinfo = db.getTypeinfo();
		int i = dbinfo.stores.indexOf(key);
		if(i >= 0) {
			store = &dbinfo.stores[i];
			return &store->object;
		}
		// No matching store, so search inside root store
		auto& root = dbinfo.stores[0].object;
		i = root.objinfo->indexOf(key);
		if(i < 0) {
			return nullptr;
		}
		store = &dbinfo.stores[0];
		return root.objinfo->data()[i];
	}

	const ConfigDB::ObjectInfo* objectSearch(const Element& element)
	{
		String key = element.getKey();

		if(element.level == 0) {
			return rootSearch(key);
		}

		auto parent = info[element.level - 1].object;
		if(!parent->objinfo) {
			return nullptr;
		}

		if(!element.container.isObject) {
			// ObjectArray defines single type
			return parent->objinfo->data()[0];
		}

		int i = parent->objinfo->indexOf(key);
		if(i >= 0) {
			return parent->objinfo->data()[i];
		}

		if(element.level == 1 && parent->isRoot()) {
			return rootSearch(key);
		}

		return nullptr;
	}

	const ConfigDB::PropertyInfo* propertySearch(const Element& element)
	{
		auto obj = info[element.level - 1].object;
		if(!obj || !obj->propinfo) {
			return nullptr;
		}

		if(element.container.isObject) {
			String key = element.getKey();
			int i = obj->propinfo->indexOf(key);
			if(i >= 0) {
				return obj->propinfo->data() + i;
			}
			return nullptr;
		}

		// Array
		return obj->propinfo->data();
	}

	bool startElement(const Element& element) override
	{
		String indent = String().pad(element.level * 2);

		String key = element.getKey();

		Serial << indent << element.level << ". " << (element.container.isObject ? "OBJ" : "ARR") << '('
			   << element.container.index << ") ";

		if(element.type == Element::Type::Object || element.type == Element::Type::Array) {
			Serial << "OBJECT '" << key << "': ";
			auto obj = objectSearch(element);
			info[element.level].object = obj;
			if(!obj) {
				Serial << _F("not in schema") << endl;
				return true;
			}
			Serial << obj->getTypeDesc() << endl;
		} else {
			Serial << "PROPERTY '" << key << "': ";
			auto prop = propertySearch(element);
			if(!prop) {
				Serial << _F("not in schema") << endl;
				return true;
			}
			String value = element.as<String>();
			if(prop->getType() == ConfigDB::PropertyType::String) {
				Format::standard.quote(value);
			}
			Serial << toString(prop->getType()) << " = " << value << endl;
		}

		// Continue parsing
		return true;
	}

	bool endElement(const Element&) override
	{
		// Continue parsing
		return true;
	}

private:
	void indentLine(unsigned level)
	{
		output << String().pad(level * 2);
	}

	ConfigDB::Database& db;
	Print& output;
	const ConfigDB::StoreInfo* store{};
	struct Info {
		const ConfigDB::ObjectInfo* object;
	};
	Info info[JSON::StreamingParser::maxNesting]{};
};

} // namespace

void parseJson(Stream& stream)
{
	BasicConfig db("test");
	ConfigListener listener(db, Serial);
	JSON::StaticStreamingParser<128> parser(&listener);
	auto status = parser.parse(stream);
	Serial << _F("Parser returned ") << toString(status) << endl;
	// return status == JSON::Status::EndOfDocument;
}
