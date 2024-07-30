/**
 * ConfigDB/Json/Store.h
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

#pragma once

#include "../Store.h"
#include <ArduinoJson.h>

namespace ConfigDB::Json
{
class Store : public ConfigDB::Store
{
public:
	/**
	 * @brief Construct a Store accessor
	 * @param db
	 * @param name JsonPATH name for store relative to database root
	 */
	Store(Database& db, const String& name) : ConfigDB::Store(db, name), doc(1024)
	{
		::Json::loadFromFile(doc, getFilename());
	}

	bool commit() override
	{
		return ::Json::saveToFile(doc, getFilename());
	}

	String getStringValue(const String& path, const String& key) const override
	{
		return getJsonObjectConst(path)[key] | nullptr;
	}

	template <typename T> bool setValue(const String& path, const String& key, const T& value)
	{
		JsonObject obj = getJsonObject(path);
		if(!obj) {
			return false;
		}
		obj[key] = value;
		return true;
	}

	template <typename T> T getValue(const String& path, const String& key, const T& defaultValue = {}) const
	{
		return getJsonObjectConst(path)[key] | defaultValue;
	}

	size_t printTo(Print& p) const override
	{
		return ::Json::serialize(doc, p);
	}

	String getFilename() const
	{
		return getPath() + ".json";
	}

private:
	friend class Object;

	/**
	 * @brief Resolve a path into the corresponding JSON object, creating it if required
	 */
	JsonObject getJsonObject(const String& path);

	JsonObjectConst getJsonObjectConst(const String& path) const;

	DynamicJsonDocument doc;
};

} // namespace ConfigDB::Json
