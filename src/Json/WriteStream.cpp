/**
 * ConfigDB/Json/WriteStream.cpp
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

#include <ConfigDB/Json/WriteStream.h>
#include <JSON/StreamingParser.h>
#include <debug_progmem.h>

namespace ConfigDB::Json
{
JSON::Status WriteStream::parse(Store& store, Stream& source)
{
	WriteStream stream;
	stream.store = &store;
	return stream.parser.parse(source);
}

bool WriteStream::startElement(const JSON::Element& element)
{
	// debug_i("%s %u %s: '%s' = %u / %s", __FUNCTION__, element.level, toString(element.type).c_str(), element.key,
	// 		element.valueLength, element.value);

	if(element.level == 0) {
		info[0] = Object(store->typeinfo(), *store);
		return true;
	}

	auto& parent = info[element.level - 1];
	if(!parent) {
		return true;
	}

	if(element.isContainer()) {
		Object obj;
		if(parent.typeinfo().type == ObjectType::ObjectArray) {
			obj = static_cast<ObjectArray&>(parent).addItem();
		} else {
			obj = parent.findObject(element.key, element.keyLength);
			if(!obj) {
				debug_w("[JSON] Object '%s' not in schema", element.key);
			}
		}
		info[element.level] = obj;
		return true;
	}

	if(parent.typeinfo().type == ObjectType::Array) {
		auto& array = static_cast<Array&>(parent);
		auto propdata = store->parseString(array.getItemType(), element.value, element.valueLength);
		array.addItem(&propdata);
		return true;
	}
	auto prop = parent.findProperty(element.key, element.keyLength);
	if(!prop) {
		debug_w("[JSON] Property '%s' not in schema", element.key);
		return true;
	}
	prop.setJsonValue(element.value, element.valueLength);
	return true;
}

size_t WriteStream::write(const uint8_t* data, size_t size)
{
	if(status != JSON::Status::Ok) {
		return 0;
	}

	status = parser.parse(reinterpret_cast<const char*>(data), size);
	return size;
}

} // namespace ConfigDB::Json
