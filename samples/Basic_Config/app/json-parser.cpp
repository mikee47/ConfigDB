#include <basic-config.h>
#include <JSON/StreamingParser.h>
#include <Data/Format/Standard.h>
#include <Data/CStringArray.h>
#include <HardwareSerial.h>
#include <ConfigDB/Pool.h>

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

	std::tuple<const ConfigDB::PropertyInfo*, uint8_t*, size_t> propertySearch(const Element& element)
	{
		auto& parent = info[element.level - 1];
		auto obj = parent.object;
		if(!obj || !obj->propinfo) {
			return {};
		}
		if(obj->getType() == ConfigDB::ObjectType::Array) {
			auto& pool = arrayPool[parent.id];
			return {obj->propinfo->data(), pool.add(), 0};
		}
		assert(obj->getType() == ConfigDB::ObjectType::Object);

		size_t offset{0};
		// Skip over child objects
		if(obj->objinfo) {
			offset += obj->objinfo->length() * sizeof(ConfigDB::ObjectId);
		}
		// Find property by key
		String key = element.getKey();
		auto data = obj->propinfo->data();
		for(unsigned i = 0; i < obj->propinfo->length(); ++i, ++data) {
			if(*data == key) {
				return {data, parent.data, offset};
			}
			offset += data->getSize();
		}
		return {};
	}

	bool startElement(const Element& element) override
	{
		String indent = String().pad(element.level * 2);

		String key = element.getKey();

		Serial << indent << element.level << ". " << (element.container.isObject ? "OBJ" : "ARR") << '('
			   << element.container.index << ") ";

		if(element.isContainer()) {
			Serial << "OBJECT '" << key << "': ";
			auto [obj, offset] = objectSearch(element);
			if(!obj) {
				Serial << _F("not in schema") << endl;
				return true;
			}
			if(obj == &store->object) {
				Serial << "{POOL} ";
				auto id = objectPool.add(obj->defaultData, obj->structSize);
				auto& pool = objectPool[id];
				info[element.level] = {obj, &pool[offset], id};
			} else {
				assert(element.level > 0);
				auto& parent = info[element.level - 1];
				switch(obj->getType()) {
				case ConfigDB::ObjectType::Object: {
					if(parent.data) {
						assert(parent.object->getType() == ConfigDB::ObjectType::Object);
						info[element.level] = {obj, parent.data + offset};
					} else {
						assert(parent.id != 0);
						auto& pool = objectArrayPool[parent.id];
						if(parent.object->getType() == ConfigDB::ObjectType::Array) {
							auto items = parent.object->propinfo->data();
							// auto id = objectArrayPool[parent.id].add(items->getSize());
							// info[element.level] = {items, nullptr, 0};
						} else if(parent.object->getType() == ConfigDB::ObjectType::ObjectArray) {
							auto items = *parent.object->objinfo->data();
							auto id = pool.add(items->defaultData, items->structSize);
							info[element.level] = {items, pool[id].get(), 0};
						} else {
							assert(false);
						}
					}
					break;
				}
				case ConfigDB::ObjectType::Array: {
					auto prop = parent.object->propinfo->data();
					auto id = arrayPool.add(prop->getSize());
					assert(parent.data);
					memcpy(parent.data + offset, &id, sizeof(id));
					Serial << "DATA " << id << endl;
					auto& pool = arrayPool[id];
					info[element.level] = {obj, nullptr, id};
					break;
				}
				case ConfigDB::ObjectType::ObjectArray: {
					auto id = objectArrayPool.add();
					assert(parent.data);
					memcpy(parent.data + offset, &id, sizeof(id));
					Serial << "DATA " << id << endl;
					auto& pool = objectArrayPool[id];
					info[element.level] = {obj, nullptr, id};
					break;
				}
				}
			}
			Serial << obj->getTypeDesc() << endl;
			return true;
		}

		Serial << "PROPERTY '" << key << "' ";
		auto [prop, data, offset] = propertySearch(element);
		if(!prop) {
			Serial << _F("not in schema") << endl;
			return true;
		}

		assert(element.level > 0);
		auto& parent = info[element.level - 1];

		Serial << '@' << String(uintptr_t(parent.data), HEX) << '[' << offset << ':' << (offset + prop->getSize())
			   << "]: ";
		String value = element.as<String>();
		ConfigDB::PropertyData propData{};
		if(prop->getType() == ConfigDB::PropertyType::String) {
			auto ref = stringPool.findOrAdd(value);
			propData.string = ref;
			Format::standard.quote(value);
		} else {
			propData.uint64 = element.as<uint64_t>();
		}
		Serial << toString(prop->getType()) << " = " << value << " (" << propData.int64 << ")" << endl;

		memcpy(data + offset, &propData, prop->getSize());
		Serial << "DATA " << propData.uint64 << ", STRINGS " << stringPool << endl;

		return true;
	}

	bool endElement(const Element&) override
	{
		// Continue parsing
		return true;
	}

	ConfigDB::ObjectPool objectPool;
	ConfigDB::ArrayPool arrayPool;
	ConfigDB::ObjectArrayPool objectArrayPool;
	ConfigDB::StringPool stringPool;

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
		uint8_t* data;
		ConfigDB::ObjectId id;
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

	auto root = reinterpret_cast<BasicConfig::Root::Struct*>(listener.objectPool[1].get());
	auto color = reinterpret_cast<BasicConfig::Color::Struct*>(listener.objectPool[2].get());
	auto events = reinterpret_cast<BasicConfig::Events::Struct*>(listener.objectPool[3].get());
	auto general = reinterpret_cast<BasicConfig::General::Struct*>(listener.objectPool[4].get());

	// return status == JSON::Status::EndOfDocument;

	Serial << "sizeof(Root) = " << sizeof(BasicConfig::Root::Struct) << endl;
	Serial << "sizeof(General) = " << sizeof(BasicConfig::General::Struct) << endl;
	Serial << "sizeof(Color) = " << sizeof(BasicConfig::Color::Struct) << endl;
	Serial << "sizeof(Events) = " << sizeof(BasicConfig::Events::Struct) << endl;
}
