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
		if(!parent.object || !parent.object->objectCount) {
			return {};
		}

		if(parent.object->type == ObjectType::ObjectArray) {
			// ObjectArray defines single type
			return {parent.object->objinfo[0], nullptr};
		}

		size_t offset{0};
		for(unsigned i = 0; i < parent.object->objectCount; ++i) {
			auto obj = parent.object->objinfo[i];
			if(obj->name.equals(element.key, size_t(element.keyLength))) {
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
			if(prop.name.equals(element.key, size_t(element.keyLength))) {
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
				auto& pool = store.arrayPool[parent.id];
				auto items = parent.object->objinfo[0];
				info[element.level] = {items, pool.add(*items), 0};
			}
			break;
		case ConfigDB::ObjectType::Array: {
			assert(obj->propertyCount == 1);
			auto id = store.arrayPool.add(obj->propinfo[0]);
			memcpy(data, &id, sizeof(id));
			info[element.level] = {obj, nullptr, id};
			break;
		}
		case ConfigDB::ObjectType::ObjectArray: {
			auto id = store.arrayPool.add(*obj->objinfo[0]);
			memcpy(data, &id, sizeof(id));
			info[element.level] = {obj, nullptr, id};
			break;
		}
		}
	}

	void handleProperty(const Element& element)
	{
		auto [prop, data] = findProperty(element);
		if(prop) {
			store.setValueString(*prop, data, element.as<String>());
		} else {
			debug_w("[JSON] '%s' not in schema", element.key);
		}
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
	String filename = getFilename();
	FileStream stream;
	if(stream.open(filename, File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<256> buffer(stream);
		printTo(buffer, 0);
	}

	if(stream.getLastError() == FS_OK) {
		debug_d("[JSON] Store saved '%s' OK", filename.c_str());
		return true;
	}

	debug_e("[JSON] Store save '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
	return false;
}

String Store::getValueJson(const PropertyInfo& info, const void* data) const
{
	String s = getValueString(info, data);
	return s ? (info.type == PropertyType::String) ? quote(s) : s : "null";
}

void Store::printObjectTo(const ObjectInfo& object, const void* data, unsigned nesting, Print& p) const
{
	auto pretty = (getDatabase().getFormat() == Format::Pretty);
	auto newline = [&]() {
		if(pretty) {
			p << endl;
		}
	};

	String indent;
	if(pretty) {
		indent.pad(nesting * 2);
	}
	bool isObject = (object.type == ObjectType::Object);
	if(nesting != 0 && !object.isRoot()) {
		if(pretty) {
			p << indent << quote(object.name) << ": ";
		} else {
			p << quote(object.name) << ':';
		}
		p << (isObject ? '{' : '[');
		newline();
	}
	unsigned item = 0;
	switch(object.type) {
	case ObjectType::Object:
		for(unsigned i = 0; i < object.objectCount; ++i) {
			auto obj = object.objinfo[i];
			if(item++) {
				p << ',';
				newline();
			}
			printObjectTo(*obj, data, nesting + 1, p);
			data = static_cast<const uint8_t*>(data) + obj->structSize;
		}
		for(unsigned i = 0; i < object.propertyCount; ++i) {
			auto& prop = object.propinfo[i];
			if(item++) {
				p << ',';
				newline();
			}
			if(pretty) {
				p << indent << "  " << quote(prop.name) << ": " << getValueJson(prop, data);
			} else {
				p << quote(prop.name) << ':' << getValueJson(prop, data);
			}
			data = static_cast<const uint8_t*>(data) + prop.getSize();
		}
		break;

	case ObjectType::Array: {
		auto id = *static_cast<const ArrayId*>(data);
		printArrayTo(object, id, nesting + 1, p);
		break;
	}

	case ObjectType::ObjectArray: {
		auto id = *static_cast<const ArrayId*>(data);
		printObjectArrayTo(object, id, nesting, p);
		break;
	}
	}

	if(nesting != 0 && !object.isRoot()) {
		if(pretty) {
			p << endl << indent;
		}
		p << (isObject ? '}' : ']');
	}
}

void Store::printArrayTo(const ObjectInfo& object, ArrayId id, unsigned nesting, Print& p) const
{
	if(id == 0) {
		return;
	}

	auto pretty = (getDatabase().getFormat() == Format::Pretty);

	String indent;
	if(pretty) {
		indent.pad(nesting * 2);
	}
	auto& array = arrayPool[id];
	auto& prop = object.propinfo[0];
	for(unsigned i = 0; i < array.getCount(); ++i) {
		if(i) {
			p << ',';
			if(pretty) {
				p << endl;
			}
		}
		if(pretty) {
			p << indent;
		}
		p << getValueJson(prop, array[i]);
	}
}

void Store::printObjectArrayTo(const ObjectInfo& object, ArrayId id, unsigned nesting, Print& p) const
{
	if(id == 0) {
		return;
	}

	auto pretty = (getDatabase().getFormat() == Format::Pretty);

	String indent;
	if(pretty) {
		indent.pad(nesting * 2);
	}
	auto& array = arrayPool[id];
	auto items = object.objinfo[0];
	for(unsigned i = 0; i < array.getCount(); ++i) {
		if(i) {
			p << ',';
			if(pretty) {
				p << endl;
			}
		}
		if(pretty) {
			p << indent << '{' << endl;
		} else {
			p << '{';
		}
		printObjectTo(*items, array[i], nesting + 1, p);
		if(pretty) {
			p << endl << indent << '}';
		} else {
			p << '}';
		}
	}
}

size_t Store::printTo(Print& p, unsigned nesting) const
{
	auto pretty = (getDatabase().getFormat() == Format::Pretty);

	size_t n{0};

	String indent;
	auto childIndent = nesting;
	if(nesting == 0) {
		p << '{';
		if(pretty) {
			p << endl;
		}
	} else if(isRoot()) {
		--childIndent;
	} else {
		if(pretty) {
			indent.pad(nesting * 2);
			p << indent << quote(getName()) << ": {" << endl;
		} else {
			p << quote(getName()) << ":{";
		}
	}

	auto& root = getTypeinfo().object;
	printObjectTo(root, rootObjectData.get(), childIndent, p);

	if(nesting == 0 || !isRoot()) {
		if(pretty) {
			p << endl << indent;
		}
		p << '}';
	}

	return n;
}

} // namespace ConfigDB::Json
