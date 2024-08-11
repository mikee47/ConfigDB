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
#include <ConfigDB/Database.h>
#include <ConfigDB/Pool.h>
#include <Data/Stream/FileStream.h>
#include <Data/CStringArray.h>
#include <Data/Buffer/PrintBuffer.h>
#include <JSON/StreamingParser.h>
#include <Data/Format/Standard.h>

#include <HardwareSerial.h>

namespace
{
String quote(String s)
{
	::Format::standard.quote(s);
	return s;
}
} // namespace

namespace ConfigDB::Json
{
class ConfigListener : public JSON::Listener
{
public:
	using Element = JSON::Element;

	ConfigListener(ConfigDB::Store& store, Print& output) : store(store), output(output)
	{
	}

	std::pair<const ConfigDB::ObjectInfo*, size_t> objectSearch(const Element& element)
	{
		if(element.level == 0) {
			return {&store.getTypeinfo().object, 0};
		}

		String key = element.getKey();

		auto parent = info[element.level - 1].object;
		if(!parent->objinfo) {
			return {};
		}

		if(!element.container.isObject) {
			// ObjectArray defines single type
			return {parent->objinfo[0], 0};
		}

		size_t offset{0};
		for(unsigned i = 0; i < parent->objectCount; ++i) {
			auto obj = parent->objinfo[i];
			if(*obj == key) {
				return {obj, offset};
			}
			offset += obj->structSize;
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
		if(obj->type == ConfigDB::ObjectType::Array) {
			auto& data = store.arrayPool[parent.id];
			return {&obj->propinfo[0], data.add(), 0};
		}
		assert(obj->type == ConfigDB::ObjectType::Object);

		size_t offset{0};
		// Skip over child objects
		for(unsigned i = 0; i < obj->objectCount; ++i) {
			offset += obj->objinfo[i]->structSize;
		}
		// Find property by key
		String key = element.getKey();
		for(unsigned i = 0; i < obj->propertyCount; ++i) {
			auto& prop = obj->propinfo[i];
			if(prop == key) {
				return {&prop, parent.data, offset};
			}
			offset += prop.getSize();
		}
		return {};
	}

	bool startElement(const Element& element) override
	{
		String indent = String().pad(element.level * 2);

		String key = element.getKey();

		// output << indent << element.level << ". " << (element.container.isObject ? "OBJ" : "ARR") << '('
		// 	   << element.container.index << ") ";

		if(element.isContainer()) {
			// output << "OBJECT '" << key << "': ";
			auto [obj, offset] = objectSearch(element);
			if(!obj) {
				// output << _F("not in schema") << endl;
				return true;
			}
			if(element.level == 0) {
				info[element.level] = {obj, store.rootObjectData.get()};
			} else {
				auto& parent = info[element.level - 1];
				switch(obj->type) {
				case ConfigDB::ObjectType::Object: {
					if(parent.data) {
						assert(parent.object->type == ConfigDB::ObjectType::Object);
						info[element.level] = {obj, parent.data + offset};
					} else {
						assert(parent.id != 0);
						auto& pool = store.objectArrayPool[parent.id];
						if(parent.object->type == ConfigDB::ObjectType::Array) {
							auto items = parent.object->propinfo[0];
							// auto id = objectArrayPool[parent.id].add(items->getSize());
							// info[element.level] = {items, nullptr, 0};
						} else if(parent.object->type == ConfigDB::ObjectType::ObjectArray) {
							auto items = parent.object->objinfo[0];
							unsigned index = pool.add(items->structSize, items->defaultData);
							info[element.level] = {items, pool[index].get(), 0};
						} else {
							assert(false);
						}
					}
					break;
				}
				case ConfigDB::ObjectType::Array: {
					auto& prop = obj->propinfo[0];
					auto id = store.arrayPool.add(prop.getSize());
					assert(parent.data);
					memcpy(parent.data + offset, &id, sizeof(id));
					// output << "DATA " << id << " @ " << String(uintptr_t(parent.data), HEX) << "+" << offset << endl;
					auto& pool = store.arrayPool[id];
					info[element.level] = {obj, nullptr, id};
					break;
				}
				case ConfigDB::ObjectType::ObjectArray: {
					auto id = store.objectArrayPool.add();
					assert(parent.data);
					memcpy(parent.data + offset, &id, sizeof(id));
					// output << "DATA " << id << endl;
					auto& pool = store.objectArrayPool[id];
					info[element.level] = {obj, nullptr, id};
					break;
				}
				}
			}
			// output << obj->getTypeDesc() << endl;
			return true;
		}

		// output << "PROPERTY '" << key << "' ";
		auto [prop, data, offset] = propertySearch(element);
		if(!prop) {
			// output << _F("not in schema") << endl;
			return true;
		}

		assert(element.level > 0);
		auto& parent = info[element.level - 1];

		// output << '@' << String(uintptr_t(data), HEX) << '[' << offset << ':' << (offset + prop->getSize()) << "]: ";
		String value = element.as<String>();
		ConfigDB::PropertyData propData{};
		if(prop->type == ConfigDB::PropertyType::String) {
			if(prop->defaultValue && *prop->defaultValue == value) {
				// output << _F("DEFAULT ");
				propData.string = 0;
			} else {
				auto ref = store.stringPool.findOrAdd(value);
				propData.string = ref;
			}
			::Format::standard.quote(value);
		} else {
			propData.uint64 = element.as<uint64_t>();
		}
		// output << toString(prop->type) << " = " << value << " (" << propData.int64 << ")" << endl;

		memcpy(data + offset, &propData, prop->getSize());
		// output << "DATA " << propData.uint64 << ", STRINGS " << store.stringPool << endl;

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
		// output << String().pad(level * 2);
	}

	ConfigDB::Store& store;
	Print& output;
	struct Info {
		const ConfigDB::ObjectInfo* object;
		uint8_t* data;
		ConfigDB::ArrayId id;
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
	auto& root = getTypeinfo().object;
	rootObjectData.reset(new uint8_t[root.structSize]);
	memcpy_P(rootObjectData.get(), root.defaultData, root.structSize);

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
	// debug_i("parsing '%s'", filename.c_str());
	ConfigListener listener(*this, Serial);
	JSON::StaticStreamingParser<128> parser(&listener);
	auto status = parser.parse(stream);
	// Serial << _F("Parser returned ") << toString(status) << endl;

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
		printTo(buffer);
	}

	if(stream.getLastError() != FS_OK) {
		debug_e("[JSON] Store save failed: %s", stream.getLastErrorString().c_str());
		return false;
	}

	return true;
}

String Store::getValueJson(const PropertyInfo& info, const void* data) const
{
	String s = getValueString(info, data);
	return s ? (info.type == PropertyType::String) ? quote(s) : s : "null";
}

void Store::printObjectTo(const ObjectInfo& object, const uint8_t* data, unsigned indentCount, Print& p) const
{
	String indent;
	indent.pad(indentCount * 2);
	bool isObject = (object.type == ObjectType::Object);
	if(object.name) {
		p << indent << quote(*object.name) << ": " << (isObject ? '{' : '[') << endl;
	}
	unsigned item = 0;
	switch(object.type) {
	case ObjectType::Object:
		for(unsigned i = 0; i < object.objectCount; ++i) {
			auto obj = object.objinfo[i];
			if(item++) {
				p << "," << endl;
			}
			printObjectTo(*obj, data, indentCount + 1, p);
			data += obj->structSize;
		}
		for(unsigned i = 0; i < object.propertyCount; ++i) {
			auto& prop = object.propinfo[i];
			if(item++) {
				p << "," << endl;
			}
			p << indent << "  " << quote(prop.getName()) << ": " << getValueJson(prop, data);
			data += prop.getSize();
		}
		break;

	case ObjectType::Array: {
		auto id = *reinterpret_cast<const ArrayId*>(data);
		printArrayTo(object, id, indentCount + 1, p);
		break;
	}

	case ObjectType::ObjectArray: {
		auto id = *reinterpret_cast<const ArrayId*>(data);
		printObjectArrayTo(object, id, indentCount + 1, p);
		break;
	}
	}

	if(object.name) {
		p << endl << indent << (isObject ? '}' : ']');
	}
}

void Store::printArrayTo(const ObjectInfo& object, ArrayId id, unsigned indentCount, Print& p) const
{
	if(id == 0) {
		return;
	}
	String indent;
	indent.pad(indentCount * 2);
	auto& array = arrayPool[id];
	auto& prop = object.propinfo[0];
	for(unsigned i = 0; i < array.getCount(); ++i) {
		if(i) {
			p << ", " << endl;
		}
		p << indent << getValueJson(prop, array[i]);
	}
}

void Store::printObjectArrayTo(const ObjectInfo& object, ArrayId id, unsigned indentCount, Print& p) const
{
	if(id == 0) {
		return;
	}
	String indent;
	indent.pad(indentCount * 2);
	auto& array = objectArrayPool[id];
	auto items = object.objinfo[0];
	for(unsigned i = 0; i < array.getCount(); ++i) {
		if(i) {
			p << ',' << endl;
		}
		p << indent << '{' << endl;
		printObjectTo(*items, array[i].get(), indentCount + 1, p);
		p << endl << indent << '}';
	}
}

size_t Store::printTo(Print& p) const
{
	auto format = getDatabase().getFormat();

	size_t n{0};

	p << "{" << endl;

	auto& root = getTypeinfo().object;
	printObjectTo(root, rootObjectData.get(), 0, p);

	p << endl << "}" << endl;

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
