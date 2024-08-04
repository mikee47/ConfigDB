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
#include <Data/CStringArray.h>

namespace ConfigDB::Json
{
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
		doc.to<JsonObject>();
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
	DeserializationError error = deserializeJson(doc, stream);
	switch(error.code()) {
	case DeserializationError::Ok:
	case DeserializationError::EmptyInput:
		debug_d("load('%s') OK, %s, %s", filename.c_str(), error.c_str(), ::Json::serialize(doc).c_str());
		return true;
	default:
		debug_e("[JSON] Store load '%s' failed: %s", filename.c_str(), error.c_str());
		doc.to<JsonObject>();
		return false;
	}
}

bool Store::save()
{
	FileStream stream;
	if(!stream.open(getFilename(), File::WriteOnly | File::CreateNewAlways)) {
		return false;
	}
	printObjectTo(doc, getDatabase().getFormat(), stream);

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

	auto root = doc.as<JsonObjectConst>();

	if(auto& name = getName()) {
		n += p.print('"');
		n += p.print(name);
		n += p.print("\":{");
		n += printObjectTo(root, format, p);
		return n;
	}

	// Nameless (root) store omits {}
	for(JsonPairConst child : root) {
		if(n > 0) {
			n += p.print(',');
		}
		n += p.print('"');
		JsonString name = child.key();
		n += p.write(name.c_str(), name.size());
		n += p.print("\":");
		JsonVariantConst value = child.value();
		n += printObjectTo(value, format, p);
	}
	return n;
}

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

} // namespace ConfigDB::Json
