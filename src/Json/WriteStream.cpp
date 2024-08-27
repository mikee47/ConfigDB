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

#include "WriteStream.h"
#include <JSON/StreamingParser.h>
#include <debug_progmem.h>

namespace ConfigDB::Json
{
WriteStream::~WriteStream()
{
	if(db && store) {
		db->save(*store);
	}
}

Status WriteStream::parse(Database& database, Stream& source)
{
	WriteStream writer(database);
	writer.parser.parse(source);
	return writer.status;
}

Status WriteStream::parse(Object& object, Stream& source)
{
	WriteStream writer(object);
	writer.parser.parse(source);
	return writer.status;
}

bool WriteStream::startElement(const JSON::Element& element)
{
	if(element.level == 0) {
		return true;
	}

	auto notInSchema = [&]() -> bool {
		debug_w("[CFGDB] '%s' not in schema", element.key);
		status.result = Result::formatError;
		return false;
	};

	auto arrayExpected = [&]() -> bool {
		debug_w("[CFGDB] '%s' not an array", element.key);
		status.result = Result::formatError;
		return false;
	};

	auto badSelector = [&]() -> bool {
		debug_w("[CFGDB] '%s' bad selector", element.key);
		status.result = Result::formatError;
		return false;
	};

	auto setProperty = [&](Property prop) -> bool {
		if(!prop) {
			return notInSchema();
		}
		const char* value = (element.type == JSON::Element::Type::Null) ? nullptr : element.value;
		if(!prop.setJsonValue(value, element.valueLength)) {
			status.result = Result::formatError;
			return false;
		}
		return true;
	};

	if(db && element.level == 1) {
		// Look in root store for a matching object
		auto& root = *db->typeinfo.stores[0];
		int i = root.findObject(element.key, element.keyLength);
		if(i >= 0) {
			store.reset();
			store = db->openStore(root, true);
			if(!store || !*store) {
				status.result = Result::updateConflict;
				return false;
			}
			info[0] = *store;
			info[1] = store->getObject(i);
			return true;
		}

		// Now check for a matching store
		if(store) {
			db->save(*store);
		}
		store.reset();
		i = db->typeinfo.findStore(element.key, element.keyLength);
		if(i < 0) {
			return notInSchema();
		}
		auto& type = *db->typeinfo.stores[i];
		store = db->openStore(type, true);
		if(!store || !*store) {
			status.result = Result::updateConflict;
			return false;
		}
		info[1] = Object(*store);
		return true;
	}

	auto& parent = info[element.level - 1];
	if(!parent) {
		return true;
	}

	auto& obj = info[element.level];

	// Check for array selector expression
	auto sel = strchr(element.key, '[');
	if(sel) {
		auto keyEnd = element.key + element.keyLength - 1;
		if(*keyEnd != ']') {
			return badSelector();
		}
		obj = parent.findObject(element.key, sel - element.key);
		if(!obj) {
			return notInSchema();
		}
		if(!obj.isArray()) {
			return arrayExpected();
		}
		auto& array = static_cast<ArrayBase&>(obj);
		int16_t len = array.getItemCount();
		if(sel + 1 == keyEnd) {
			// Append only
			if(!element.isContainer()) {
				if(array.typeIs(ObjectType::ObjectArray)) {
					return badSelector();
				}
				return setProperty(static_cast<Array&>(array).addItem());
			}
			obj.streamPos = len;
			return true;
		}

		auto sep = strchr(sel, '=');
		if(sep) {
			// name=value selector
			auto name = sel + 1;
			auto namelen = sep - name;
			auto value = sep + 1;
			auto valuelen = element.key + element.keyLength - value - 1;
			arrayParent = static_cast<ObjectArray&>(obj);
			auto propIndex = arrayParent.getItemType().findProperty(name, namelen);
			if(propIndex < 0) {
				return badSelector();
			}
			for(unsigned i = 0; i < array.getItemCount(); ++i) {
				auto item = arrayParent.getItem(i);
				auto prop = item.getProperty(propIndex);
				auto propValue = prop.getValue();
				if(propValue.equals(value, valuelen)) {
					obj = item;
					return true;
				}
			}
			return badSelector();
		}

		char* ptr;
		int16_t start = strtol(sel + 1, &ptr, 0);
		if(start < 0) {
			start += len;
		}
		if(ptr == keyEnd) {
			// Single index, value can be a property or object
			if(start >= len) {
				return badSelector();
			}
			// If its an object we set info and finish
			if(obj.typeIs(ObjectType::ObjectArray)) {
				if(!element.isContainer()) {
					return badSelector();
				}
				arrayParent = static_cast<ObjectArray&>(obj);
				obj = arrayParent.getObject(start);
				return true;
			}
			// If its a property we can set it now
			return setProperty(array.getProperty(start));
		}

		// Range
		int16_t end = -1;
		if(*ptr == ':') {
			start = std::min(start, len);
			if(ptr[1] == ']') {
				end = len;
				++ptr;
			} else {
				end = strtol(ptr + 1, &ptr, 0);
				if(end < 0) {
					end += len;
				}
			}
		}
		if(ptr != keyEnd) {
			return badSelector();
		}
		// Range
		if(element.type != JSON::Element::Type::Array) {
			return arrayExpected();
		}
		// Remove items in range
		while(end > start) {
			array.removeItem(start);
			--end;
		}
		obj.streamPos = start; // Where to insert new items
		return true;
	}

	if(element.isContainer()) {
		if(parent.typeIs(ObjectType::ObjectArray)) {
			obj = static_cast<ObjectArray&>(parent).insertItem(parent.streamPos++);
		} else {
			obj = parent.findObject(element.key, element.keyLength);
			if(!obj) {
				return notInSchema();
			}
			if(obj.isArray()) {
				static_cast<ArrayBase&>(obj).clear();
			}
		}
		return true;
	}

	if(parent.typeIs(ObjectType::Array)) {
		auto& array = static_cast<Array&>(parent);
		return setProperty(array.insertItem(parent.streamPos++));
	}

	return setProperty(parent.findProperty(element.key, element.keyLength));
}

size_t WriteStream::write(const uint8_t* data, size_t size)
{
	if(!status) {
		return 0;
	}

	auto jsonStatus = parser.parse(reinterpret_cast<const char*>(data), size);
	switch(jsonStatus) {
	case JSON::Status::Ok:
	case JSON::Status::EndOfDocument:
	case JSON::Status::Cancelled:
		break;
	default:
		status.result = Result::formatError;
	}
	return size;
}

} // namespace ConfigDB::Json
