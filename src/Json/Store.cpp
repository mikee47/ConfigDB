/**
 * ConfigDB/Json/Store.cpp
 *
 * Copyright 2024 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the ConfigDB Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#include <ConfigDB/Json/Store.h>
#include <ConfigDB/Json/Object.h>
#include <ConfigDB/Database.h>
#include <ConfigDB/Pool.h>
#include <Data/Stream/FileStream.h>
#include <Data/CStringArray.h>
#include <Data/Buffer/PrintBuffer.h>
#include <JSON/StreamingParser.h>
#include <Data/Format/Standard.h>

#include <HardwareSerial.h>

namespace ConfigDB::Json
{
class ConfigListener : public JSON::Listener
{
public:
	using Element = JSON::Element;

	ConfigListener(ConfigDB::Store& store, Print& output) : store(store), output(output)
	{
	}

	/*
	 * Top level key can match against a store name or an entry in the root object.
	 * Return the matching object, or nullptr.
	 */
	// std::pair<const ConfigDB::ObjectInfo*, size_t> rootSearch(const String& key)
	// {
	// 	// Find store
	// 	auto& dbinfo = db.getTypeinfo();
	// 	int i = dbinfo.stores.indexOf(key);
	// 	if(i >= 0) {
	// 		store = &dbinfo.stores[i];
	// 		return {&store->object, 0};
	// 	}
	// 	// No matching store, so search inside root store
	// 	auto& root = dbinfo.stores[0].object;
	// 	size_t offset{0};
	// 	for(auto& obj : *root.objinfo) {
	// 		if(obj == key) {
	// 			store = &dbinfo.stores[0];
	// 			return {&obj, offset};
	// 		}
	// 		offset += obj.getStructSize();
	// 	}
	// 	return {};
	// }

	std::pair<const ConfigDB::ObjectInfo*, size_t> objectSearch(const Element& element)
	{
		if(element.level == 0){
			return {&store.getTypeinfo().object, 0};
		}

		String key = element.getKey();

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
			offset += obj.getStructSize();
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

		output << indent << element.level << ". " << (element.container.isObject ? "OBJ" : "ARR") << '('
			   << element.container.index << ") ";

		if(element.isContainer()) {
			output << "OBJECT '" << key << "': ";
			auto [obj, offset] = objectSearch(element);
			if(!obj) {
				output << _F("not in schema") << endl;
				return true;
			}
			if(element.level == 0) {
				output << "{POOL} ";
				auto id = objectPool.add(obj->defaultData, obj->getStructSize());
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
							auto id = pool.add(items->defaultData, items->getStructSize());
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
					output << "DATA " << id << endl;
					auto& pool = arrayPool[id];
					info[element.level] = {obj, nullptr, id};
					break;
				}
				case ConfigDB::ObjectType::ObjectArray: {
					auto id = objectArrayPool.add();
					assert(parent.data);
					memcpy(parent.data + offset, &id, sizeof(id));
					output << "DATA " << id << endl;
					auto& pool = objectArrayPool[id];
					info[element.level] = {obj, nullptr, id};
					break;
				}
				}
			}
			output << obj->getTypeDesc() << endl;
			return true;
		}

		output << "PROPERTY '" << key << "' ";
		auto [prop, data, offset] = propertySearch(element);
		if(!prop) {
			output << _F("not in schema") << endl;
			return true;
		}

		assert(element.level > 0);
		auto& parent = info[element.level - 1];

		output << '@' << String(uintptr_t(parent.data), HEX) << '[' << offset << ':' << (offset + prop->getSize())
			   << "]: ";
		String value = element.as<String>();
		ConfigDB::PropertyData propData{};
		if(prop->getType() == ConfigDB::PropertyType::String) {
			if(prop->defaultValue && *prop->defaultValue == value) {
				output << _F("DEFAULT ");
				propData.string = 0;
			} else {
				auto ref = stringPool.findOrAdd(value);
				propData.string = ref;
			}
			::Format::standard.quote(value);
		} else {
			propData.uint64 = element.as<uint64_t>();
		}
		output << toString(prop->getType()) << " = " << value << " (" << propData.int64 << ")" << endl;

		memcpy(data + offset, &propData, prop->getSize());
		output << "DATA " << propData.uint64 << ", STRINGS " << stringPool << endl;

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

	ConfigDB::Store& store;
	Print& output;
	struct Info {
		const ConfigDB::ObjectInfo* object;
		uint8_t* data;
		ConfigDB::ObjectId id;
	};
	Info info[JSON::StreamingParser::maxNesting]{};
};

/**
 * TODO: How to handle failures?
 *
 * Any load failures here result in no JsonDocument being available.
 * Write/save operations will fail, and read operations will return default values.
 *
 * Recovery options:
 * 	- Revert store to default values and continue
 *  - On incomplete reads we can use values obtained at risk of inconsistencies
 */
bool Store::load()
{
	String filename = getFilename();
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		if(stream.getLastError() != IFS::Error::NotFound) {
			debug_w("open('%s') failed", filename.c_str());
		}
		// Create new document
		// doc.to<JsonObject>();
		return true;
	}

	/*
	 * If deserialization fails wipe document and fail.
	 * All values for this store thus revert to their defaults.
	 *
	 * TODO: Find a way to report this sort of problem to the user and indicate whether to proceed
	 *
	 * We want to load store only when needed to minimise RAM usage.
	 */
	debug_i("parsing '%s'", filename.c_str());
	ConfigListener listener(*this, Serial);
	JSON::StaticStreamingParser<128> parser(&listener);
	auto status = parser.parse(stream);
	Serial << _F("Parser returned ") << toString(status) << endl;

	// DeserializationError error = deserializeJson(doc, stream);
	// switch(error.code()) {
	// case DeserializationError::Ok:
	// case DeserializationError::EmptyInput:
	// 	debug_d("load('%s') OK, %s, %s", filename.c_str(), error.c_str(), ::Json::serialize(doc).c_str());
	// 	return true;
	// default:
	// 	debug_e("[JSON] Store load '%s' failed: %s", filename.c_str(), error.c_str());
	// 	doc.to<JsonObject>();
	// 	return false;
	// }

	return false;
}

bool Store::save()
{
	FileStream stream;
	if(!stream.open(getFilename(), File::WriteOnly | File::CreateNewAlways)) {
		return false;
	}

	{
		StaticPrintBuffer<256> buffer(stream);
		// printObjectTo(doc, getDatabase().getFormat(), buffer);
	}

	if(stream.getLastError() != FS_OK) {
		debug_e("[JSON] Store save failed: %s", stream.getLastErrorString().c_str());
		return false;
	}

	return true;
}

size_t Store::printTo(Print& p) const
{
	auto format = getDatabase().getFormat();

	size_t n{0};

	// auto root = doc.as<JsonObjectConst>();

	// if(name) {
	// 	n += p.print('"');
	// 	n += p.print(name);
	// 	n += p.print("\":");
	// 	n += printObjectTo(root, format, p);
	// 	return n;
	// }

	// // Nameless (root) store omits {}
	// for(JsonPairConst child : root) {
	// 	if(n > 0) {
	// 		n += p.print(',');
	// 		if(format == Format::Pretty) {
	// 			n += p.println();
	// 		}
	// 	}
	// 	n += p.print('"');
	// 	JsonString name = child.key();
	// 	n += p.write(name.c_str(), name.size());
	// 	n += p.print("\":");
	// 	JsonVariantConst value = child.value();
	// 	n += printObjectTo(value, format, p);
	// }
	return n;
}

#if 0

JsonObject Store::getJsonObject(const String& path)
{
	String s(path);
	s.replace('.', '\0');
	CStringArray csa(std::move(s));
	auto obj = doc.isNull() ? doc.to<JsonObject>() : doc.as<JsonObject>();
	for(auto key : csa) {
		if(!obj) {
			break;
		}
		auto child = obj[key];
		if(!child.is<JsonObject>()) {
			child = obj.createNestedObject(const_cast<char*>(key));
		}
		obj = child;
	}
	return obj;
}

JsonObjectConst Store::getJsonObjectConst(const String& path) const
{
	String s(path);
	s.replace('.', '\0');
	CStringArray csa(std::move(s));
	auto obj = doc.as<JsonObjectConst>();
	for(auto key : csa) {
		if(!obj) {
			break;
		}
		auto child = obj[key];
		if(!child.is<JsonObjectConst>()) {
			obj = {};
			break;
		}
		obj = child;
	}

	return obj;
}

JsonArray Store::getJsonArray(const String& path)
{
	String s(path);
	s.replace('.', '\0');
	CStringArray csa(std::move(s));
	auto name = csa.popBack();
	auto obj = doc.isNull() ? doc.to<JsonObject>() : doc.as<JsonObject>();
	for(auto key : csa) {
		if(!obj) {
			break;
		}
		auto child = obj[key];
		if(!child.is<JsonObject>()) {
			child = obj.createNestedObject(const_cast<char*>(key));
		}
		obj = child;
	}

	JsonArray arr = obj[name];
	if(!arr) {
		arr = obj.createNestedArray(name);
	}
	return arr;
}

JsonArrayConst Store::getJsonArrayConst(const String& path) const
{
	String s(path);
	s.replace('.', '\0');
	CStringArray csa(std::move(s));
	auto name = csa.popBack();
	auto obj = doc.as<JsonObjectConst>();
	for(auto key : csa) {
		if(!obj) {
			break;
		}
		auto child = obj[key];
		if(!child.is<JsonObjectConst>()) {
			obj = {};
			break;
		}
		obj = child;
	}

	return obj[name];
}

#endif

} // namespace ConfigDB::Json
