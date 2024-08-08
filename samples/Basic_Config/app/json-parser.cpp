#include <basic-config.h>
#include <JSON/StreamingParser.h>
#include <Data/Format/Standard.h>
#include <Data/CStringArray.h>
#include <HardwareSerial.h>

namespace
{
class ConfigListener : public JSON::Listener
{
public:
	using Element = JSON::Element;

	ConfigListener(ConfigDB::Database& db, Print& output) : db(db), output(output)
	{
		memset(static_cast<void*>(&rootData), 0, sizeof(rootData));
	}

	/*
	 * Top level key can match against a store name or an entry in the root object.
	 * Return the matching object, or nullptr.
	 */
	std::pair<const ConfigDB::ObjectInfo*, size_t> rootSearch(const String& key)
	{
		// Find store
		auto& dbinfo = db.getTypeinfo();
		int i = dbinfo.stores.indexOf(key);
		if(i >= 0) {
			store = &dbinfo.stores[i];
			return {&store->object, 0};
		}
		// No matching store, so search inside root store
		auto& root = dbinfo.stores[0].object;
		size_t offset{0};
		for(auto& obj : *root.objinfo) {
			if(obj == key) {
				store = &dbinfo.stores[0];
				return {&obj, offset};
			}
			offset += obj.structSize;
		}
		return {};
	}

	std::pair<const ConfigDB::ObjectInfo*, size_t> objectSearch(const Element& element)
	{
		String key = element.getKey();

		if(element.level == 0) {
			return rootSearch(key);
		}

		auto parent = info[element.level - 1].object;
		if(!parent->objinfo) {
			return {};
		}

		if(!element.container.isObject) {
			// ObjectArray defines single type
			return {parent->objinfo->data()[0], 0};
		}

		size_t offset{0};
		for(auto& obj : *parent->objinfo) {
			if(obj == key) {
				return {&obj, offset};
			}
			offset += obj.structSize;
		}

		// NOTE: In practice this never happens since we load ONE store only
		if(element.level == 1 && parent->isRoot()) {
			return rootSearch(key);
		}

		return {};
	}

	std::pair<const ConfigDB::PropertyInfo*, size_t> propertySearch(const Element& element)
	{
		auto obj = info[element.level - 1].object;
		if(!obj || !obj->propinfo) {
			return {};
		}

		if(element.container.isObject) {
			String key = element.getKey();
			size_t offset{0};
			auto data = obj->propinfo->data();
			for(unsigned i = 0; i < obj->propinfo->length(); ++i, ++data) {
				if(*data == key) {
					return {data, offset};
				}
				offset += data->getSize();
			}
			return {};
		}

		// Array
		return {obj->propinfo->data(), 0};
	}

	bool startElement(const Element& element) override
	{
		String indent = String().pad(element.level * 2);

		String key = element.getKey();

		Serial << indent << element.level << ". " << (element.container.isObject ? "OBJ" : "ARR") << '('
			   << element.container.index << ") ";

		if(element.type == Element::Type::Object || element.type == Element::Type::Array) {
			Serial << "OBJECT '" << key << "': ";
			auto [obj, offset] = objectSearch(element);
			info[element.level] = {obj, offset};
			if(element.level > 0) {
				info[element.level].offset += info[element.level - 1].offset;
			}
			if(!obj) {
				Serial << _F("not in schema") << endl;
				return true;
			}
			Serial << obj->getTypeDesc() << endl;
		} else {
			Serial << "PROPERTY '" << key << "' ";
			auto [prop, offset] = propertySearch(element);
			if(!prop) {
				Serial << _F("not in schema") << endl;
				return true;
			}
			if(element.level > 0) {
				offset += info[element.level - 1].offset;
			}
			Serial << "@ " << offset << '[' << prop->getSize() << "]: ";
			String value = element.as<String>();
			ConfigDB::PropertyData data{};
			if(prop->getType() == ConfigDB::PropertyType::String) {
				int i = stringPool.indexOf(value);
				if(i < 0) {
					i = stringPool.count();
					stringPool += value;
				}
				data.string = 1 + i;
				Format::standard.quote(value);
			} else {
				data.uint64 = element.as<uint64_t>();
			}
			Serial << toString(prop->getType()) << " = " << value << endl;

			if(store->isRoot()) {
				memcpy(reinterpret_cast<uint8_t*>(&rootData) + offset, &data, prop->getSize());
				Serial << "DATA " << data.uint64 << ", STRINGS " << stringPool.join() << endl;
			}
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
	BasicConfig::Root::Struct rootData;
	CStringArray stringPool;
	const ConfigDB::StoreInfo* store{};
	struct Info {
		const ConfigDB::ObjectInfo* object;
		size_t offset;
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

	Serial << "sizeof(Root) = " << sizeof(BasicConfig::Root::Struct) << endl;
	Serial << "sizeof(General) = " << sizeof(BasicConfig::General::Struct) << endl;
	Serial << "sizeof(Color) = " << sizeof(BasicConfig::Color::Struct) << endl;
	Serial << "sizeof(Events) = " << sizeof(BasicConfig::Events::Struct) << endl;
}
