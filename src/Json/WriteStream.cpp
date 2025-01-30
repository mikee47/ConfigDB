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
Status WriteStream::getStatus() const
{
	switch(jsonStatus) {
	case JSON::Status::EndOfDocument:
	case JSON::Status::Cancelled:
		return status;
	case JSON::Status::Ok:
	default:
		return Status::formatError(FormatError::BadSyntax);
	}
}

Status WriteStream::parse(Database& database, Stream& source)
{
	WriteStream writer(database);
	writer.jsonStatus = writer.parser.parse(source);
	return writer.getStatus();
}

Status WriteStream::parse(Object& object, Stream& source)
{
	WriteStream writer(object);
	writer.jsonStatus = writer.parser.parse(source);
	return writer.getStatus();
}

bool WriteStream::handleError(FormatError err, Object& object, const String& arg)
{
	status = err;
	Database& db = database ? *database : info[0].getDatabase();
	return db.handleFormatError(err, object, arg);
	return true;
}

bool WriteStream::handleError(FormatError err, const String& arg)
{
	Object object;
	return handleError(err, object, arg);
}

bool WriteStream::openStore(unsigned storeIndex)
{
	if(store && database->typeinfo.indexOf(store->propinfo()) == int(storeIndex)) {
		return true;
	}
	store = StoreUpdateRef();
	store = database->openStoreForUpdate(storeIndex);
	return bool(store);
}

bool WriteStream::startElement(const Element& element)
{
	if(element.level == 0) {
		if(!element.isContainer()) {
			return handleError(FormatError::BadType, toString(element.type));
		}
		return true;
	}

	auto& parent = info[element.level - 1];

	// Check for array selector expression
	auto sel = strchr(element.key, '[');

	if(database && element.level == 1) {
		if(!sel && element.type == Element::Type::Object) {
			return locateStoreOrRoot(element);
		}
		// Assume this is a property of the root store
		if(!openStore(0)) {
			return handleError(FormatError::UpdateConflict, element.getKey());
		}
		parent = *store;
	}

	if(sel) {
		return handleSelector(element, sel);
	}

	auto& obj = info[element.level];

	if(parent.typeIs(ObjectType::ObjectArray)) {
		if(element.type != Element::Type::Object) {
			return handleError(FormatError::BadType, parent, toString(element.type));
		}
		obj = static_cast<ObjectArray&>(parent).insertItem(parent.streamPos++);
		return true;
	}

	if(parent.typeIs(ObjectType::Array)) {
		if(element.isContainer()) {
			return handleError(FormatError::BadType, parent, toString(element.type));
		}
		auto& array = static_cast<Array&>(parent);
		return setProperty(element, array, array.insertItem(parent.streamPos++));
	}

	if(element.isContainer()) {
		obj = parent.findObject(element.key, element.keyLength);
		if(!obj) {
			return handleError(FormatError::NotInSchema, parent, element.getKey());
		}
		if(obj.isArray()) {
			static_cast<ArrayBase&>(obj).clear();
		}
		return true;
	}

	auto prop = parent.findProperty(element.key, element.keyLength);
	if(!prop) {
		return handleError(FormatError::NotInSchema, parent, element.getKey());
	}
	return setProperty(element, parent, prop);
}

/*
 * When processing a full database the element at level #1 can refer to either a store
 * or an object in the Root store.
 */
bool WriteStream::locateStoreOrRoot(const Element& element)
{
	auto& parent = info[element.level - 1];
	auto& obj = info[element.level];
	obj = {};

	// Look in root store for a matching object
	auto& root = database->typeinfo.stores[0];
	int i = root.findObject(element.key, element.keyLength);
	if(i >= 0) {
		if(!openStore(0)) {
			return handleError(FormatError::UpdateConflict, element.getKey());
		}
		parent = *store;
		obj = store->getObject(i);
		return true;
	}

	// Now check for a matching store
	store = StoreUpdateRef();
	i = database->typeinfo.findStore(element.key, element.keyLength);
	if(i < 0) {
		return handleError(FormatError::NotInSchema, element.getKey());
	}
	if(!openStore(i)) {
		return handleError(FormatError::UpdateConflict, element.getKey());
	}
	obj = *store;
	return true;
}

/*
 * Array selectors are the optional part in a key name between [].
 * There are several forms to consider:
 *
 * 		x[]
 * 		x[0]
 * 		x[0:]
 * 		x[0:1]
 * 		x[name=value]
 */
bool WriteStream::handleSelector(const Element& element, const char* sel)
{
	auto& parent = info[element.level - 1];
	auto& obj = info[element.level];
	obj = {};

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
			return setProperty(element, array, static_cast<Array&>(array).addItem());
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
				if(element.type == Element::Type::Array) {
					// Convert named selector into range
					array.removeItem(i);
					obj.streamPos = i; // Where to insert new items
				} else {
					obj = item;
				}
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
		return setProperty(element, array, prop);
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

bool WriteStream::setProperty(const Element& element, Object& object, Property prop)
{
	const char* value = (element.type == Element::Type::Null) ? nullptr : element.value;
	if(!prop.setJsonValue(value, element.valueLength)) {
		return handleError(FormatError::BadProperty, object, String(element.value, element.valueLength));
	}
	return true;
}

size_t WriteStream::write(const uint8_t* data, size_t size)
{
	if(jsonStatus != JSON::Status::Ok) {
		return 0;
	}

	jsonStatus = parser.parse(reinterpret_cast<const char*>(data), size);
	return size;
}

} // namespace ConfigDB::Json
