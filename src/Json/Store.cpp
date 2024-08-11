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

	ConfigListener(ConfigDB::Store& store) : store(store)
	{
	}

	std::pair<const ConfigDB::ObjectInfo*, uint8_t*> findObject(const Element& element)
	{
		auto& parent = info[element.level - 1];
		if(!parent.object->objectCount) {
			return {};
		}

		if(parent.object->type == ObjectType::ObjectArray) {
			// ObjectArray defines single type
			return {parent.object->objinfo[0], nullptr};
		}

		size_t offset{0};
		for(unsigned i = 0; i < parent.object->objectCount; ++i) {
			auto obj = parent.object->objinfo[i];
			if(obj->nameIs(element.key, element.keyLength)) {
				return {obj, parent.data + offset};
			}
			offset += obj->structSize;
		}

		return {};
	}

	std::tuple<const ConfigDB::PropertyInfo*, uint8_t*> findProperty(const Element& element)
	{
		auto& parent = info[element.level - 1];
		auto obj = parent.object;
		if(!obj || !obj->propertyCount) {
			return {};
		}
		if(obj->type == ConfigDB::ObjectType::Array) {
			auto& data = store.arrayPool[parent.id];
			return {&obj->propinfo[0], data.add()};
		}
		assert(obj->type == ConfigDB::ObjectType::Object);

		size_t offset{0};
		// Skip over child objects
		for(unsigned i = 0; i < obj->objectCount; ++i) {
			offset += obj->objinfo[i]->structSize;
		}
		// Find property by key
		for(unsigned i = 0; i < obj->propertyCount; ++i) {
			auto& prop = obj->propinfo[i];
			if(prop.nameIs(element.key, element.keyLength)) {
				return {&prop, parent.data + offset};
			}
			offset += prop.getSize();
		}
		return {};
	}

	void handleContainer(const Element& element)
	{
		if(element.level == 0) {
			auto obj = &store.getTypeinfo().object;
			info[element.level] = {obj, store.rootObjectData.get()};
			return;
		}

		auto [obj, data] = findObject(element);
		if(!obj) {
			debug_w("[JSON] '%s' not in schema", element.key);
			return;
		}

		auto& parent = info[element.level - 1];
		switch(obj->type) {
		case ConfigDB::ObjectType::Object:
			if(data) {
				assert(parent.object->type == ConfigDB::ObjectType::Object);
				info[element.level] = {obj, data};
			} else {
				assert(parent.object->type == ConfigDB::ObjectType::ObjectArray);
				assert(parent.id != 0);
				auto& pool = store.objectArrayPool[parent.id];
				auto items = parent.object->objinfo[0];
				unsigned index = pool.add(items->structSize, items->defaultData);
				info[element.level] = {items, pool[index].get(), 0};
			}
			break;
		case ConfigDB::ObjectType::Array: {
			auto& prop = obj->propinfo[0];
			auto id = store.arrayPool.add(prop.getSize());
			memcpy(data, &id, sizeof(id));
			auto& pool = store.arrayPool[id];
			info[element.level] = {obj, nullptr, id};
			break;
		}
		case ConfigDB::ObjectType::ObjectArray: {
			auto id = store.objectArrayPool.add();
			memcpy(data, &id, sizeof(id));
			auto& pool = store.objectArrayPool[id];
			info[element.level] = {obj, nullptr, id};
			break;
		}
		}
	}

	void handleProperty(const Element& element)
	{
		auto [prop, data] = findProperty(element);
		if(!prop) {
			debug_w("[JSON] '%s' not in schema", element.key);
			return;
		}

		assert(element.level > 0);
		auto& parent = info[element.level - 1];

		String value = element.as<String>();
		ConfigDB::PropertyData propData{};
		if(prop->type == ConfigDB::PropertyType::String) {
			if(prop->defaultValue && *prop->defaultValue == value) {
				propData.string = 0;
			} else {
				auto ref = store.stringPool.findOrAdd(value);
				propData.string = ref;
			}
			::Format::standard.quote(value);
		} else {
			propData.uint64 = element.as<uint64_t>();
		}

		memcpy(data, &propData, prop->getSize());
	}

	bool startElement(const Element& element) override
	{
		if(element.isContainer()) {
			handleContainer(element);
		} else {
			handleProperty(element);
		}
		return true;
	}

	bool endElement(const Element&) override
	{
		// Continue parsing
		return true;
	}

private:
	ConfigDB::Store& store;
	struct Info {
		const ConfigDB::ObjectInfo* object;
		uint8_t* data;
		ConfigDB::ArrayId id;
	};
	Info info[JSON::StreamingParser::maxNesting]{};
};

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
		return true;
	}

	ConfigListener listener(*this);
	JSON::StaticStreamingParser<128> parser(&listener);
	auto status = parser.parse(stream);

	if(status == JSON::Status::EndOfDocument) {
		return true;
	}

	debug_w("JSON load '%s': %s", filename.c_str(), toString(status).c_str());
	return false;
}

bool Store::save()
{
	FileStream stream;
	if(stream.open(getFilename(), File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<256> buffer(stream);
		printTo(buffer);
	}

	if(stream.getLastError() == FS_OK) {
		return true;
	}

	debug_e("[JSON] Store save failed: %s", stream.getLastErrorString().c_str());
	return false;
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

} // namespace ConfigDB::Json
