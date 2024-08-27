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

JSON::Status WriteStream::parse(Database& database, Stream& source)
{
	return WriteStream(database).parser.parse(source);
}

JSON::Status WriteStream::parse(Object& object, Stream& source)
{
	return WriteStream(object).parser.parse(source);
}

bool WriteStream::startElement(const JSON::Element& element)
{
	if(element.level == 0) {
		return true;
	}

	auto notInSchema = [&element]() -> bool {
		debug_e("[JSON] '%s' not in schema", element.key);
		return false;
	};

	auto setProperty = [&](Property prop) -> bool {
		if(!prop) {
			return notInSchema();
		}
		const char* value = (element.type == JSON::Element::Type::Null) ? nullptr : element.value;
		return prop.setJsonValue(value, element.valueLength);
	};

	if(db && element.level == 1) {
		// Look in root store for a matching object
		auto& root = *db->typeinfo.stores[0];
		int i = root.findObject(element.key, element.keyLength);
		if(i >= 0) {
			store.reset();
			store = db->openStore(root, true);
			if(!store || !*store) {
				// Fatal: store is locked
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
			// Fatal: store is locked
			return false;
		}
		info[1] = Object(*store);
		return true;
	}

	auto& parent = info[element.level - 1];
	if(!parent) {
		return true;
	}

	if(element.isContainer()) {
		Object obj;
		if(parent.typeIs(ObjectType::ObjectArray)) {
			obj = static_cast<ObjectArray&>(parent).addItem();
		} else {
			obj = parent.findObject(element.key, element.keyLength);
			if(!obj) {
				return notInSchema();
			}
			if(obj.isArray()) {
				static_cast<ArrayBase&>(obj).clear();
			}
		}
		info[element.level] = obj;
		return true;
	}

	if(parent.typeIs(ObjectType::Array)) {
		auto& array = static_cast<Array&>(parent);
		return setProperty(array.addProperty());
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
	case JSON::Status::EndOfDocument:
		break;
	case JSON::Status::Cancelled:
		status.result = Result::updateConflict;
		break;
	default:
		status.result = Result::formatError;
	}
	return size;
}

} // namespace ConfigDB::Json
