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

using Element = JSON::Element;

namespace ConfigDB::Json
{
WriteStream::~WriteStream()
{
	if(database && store) {
		database->save(*store);
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

bool WriteStream::handleError(FormatError err, Object& object, const String& arg)
{
	status = err;
	Database& db = database ? *database : info[0].getDatabase();
	return db.handleFormatError(err, object, arg);
	return true;
}

bool WriteStream::startElement(const Element& element)
{
	if(element.level == 0) {
		if(element.type != Element::Type::Object) {
			return handleError(FormatError::BadType, toString(element.type));
		}
		return true;
	}

	auto setProperty = [&](Object& object, Property prop) -> bool {
		const char* value = (element.type == Element::Type::Null) ? nullptr : element.value;
		if(!prop.setJsonValue(value, element.valueLength)) {
			return handleError(FormatError::SetPropFailed, object, String(element.value, element.valueLength));
		}
		return true;
	};

	auto& parent = info[element.level - 1];
	auto& obj = info[element.level];
	obj = {};

	if(database && element.level == 1) {
		// Look in root store for a matching object
		auto& root = *database->typeinfo.stores[0];
		int i = root.findObject(element.key, element.keyLength);
		if(i >= 0) {
			store.reset();
			store = database->openStore(root, true);
			if(!store || !*store) {
				return handleError(FormatError::UpdateConflict, element.getKey());
			}
			parent = *store;
			obj = store->getObject(i);
			return true;
		}

		// Now check for a matching store
		if(store) {
			database->save(*store);
		}
		store.reset();
		i = database->typeinfo.findStore(element.key, element.keyLength);
		if(i < 0) {
			return handleError(FormatError::NotInSchema, element.getKey());
		}
		auto& type = *database->typeinfo.stores[i];
		store = database->openStore(type, true);
		if(!store || !*store) {
			return handleError(FormatError::UpdateConflict, type.name);
		}
		obj = *store;
		return true;
	}

	// Check for array selector expression
	auto sel = strchr(element.key, '[');
	if(sel) {
		auto keyEnd = element.key + element.keyLength - 1;
		if(*keyEnd != ']') {
			return handleError(FormatError::BadSelector, parent, element.getKey());
		}
		obj = parent.findObject(element.key, sel - element.key);
		if(!obj) {
			return handleError(FormatError::NotInSchema, parent, element.getKey());
		}
		if(!obj.isArray()) {
			return handleError(FormatError::BadType, obj, toString(obj.typeinfo().type));
		}
		auto& array = static_cast<ArrayBase&>(obj);
		int16_t len = array.getItemCount();
		++sel;
		if(sel == keyEnd) {
			// Append only
			if(!element.isContainer()) {
				if(!array.typeIs(ObjectType::Array)) {
					return handleError(FormatError::BadType, array, toString(element.type));
				}
				return setProperty(array, static_cast<Array&>(array).addItem());
			}
			if(element.type == Element::Type::Object && !array.typeIs(ObjectType::ObjectArray)) {
				return handleError(FormatError::BadType, array, toString(element.type));
			}
			obj.streamPos = len;
			return true;
		}

		auto sep = strchr(sel, '=');
		if(sep) {
			// name=value selector
			auto name = sel;
			auto namelen = sep - name;
			auto value = sep + 1;
			auto valuelen = keyEnd - value;
			arrayParent = static_cast<ObjectArray&>(obj);
			auto propIndex = arrayParent.getItemType().findProperty(name, namelen);
			if(propIndex < 0) {
				return handleError(FormatError::NotInSchema, obj, String(name, namelen));
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
			return handleError(FormatError::BadSelector, obj, String(value, valuelen));
		}

		char* ptr;
		int16_t start = strtol(sel, &ptr, 0);
		if(start < 0) {
			start += len;
		}
		if(ptr == keyEnd) {
			// Single index, value can be a property or object
			if(start >= len) {
				return handleError(FormatError::BadIndex, obj, String(start));
			}
			// If its an object we set info and finish
			if(obj.typeIs(ObjectType::ObjectArray)) {
				if(!element.isContainer()) {
					return handleError(FormatError::BadType, obj, element.getKey());
				}
				arrayParent = static_cast<ObjectArray&>(obj);
				obj = arrayParent.getObject(start);
				return true;
			}
			// If its a property we can set it now
			auto prop = array.getProperty(start);
			if(!prop) {
				return handleError(FormatError::NotInSchema, array, String(start));
			}
			return setProperty(array, prop);
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
			return handleError(FormatError::BadSelector, array, String(sel, keyEnd - sel));
		}
		// Range
		if(element.type != Element::Type::Array) {
			return handleError(FormatError::BadType, array, toString(element.type));
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
				return handleError(FormatError::NotInSchema, parent, element.getKey());
			}
			if(obj.isArray()) {
				static_cast<ArrayBase&>(obj).clear();
			}
		}
		return true;
	}

	if(parent.typeIs(ObjectType::Array)) {
		auto& array = static_cast<Array&>(parent);
		return setProperty(array, array.insertItem(parent.streamPos++));
	}

	auto prop = parent.findProperty(element.key, element.keyLength);
	if(!prop) {
		return handleError(FormatError::NotInSchema, parent, element.getKey());
	}
	return setProperty(parent, prop);
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
		status = FormatError{};
	}
	return size;
}

} // namespace ConfigDB::Json
