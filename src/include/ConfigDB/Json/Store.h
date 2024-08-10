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
#include "Object.h"
#include "Array.h"
#include "ObjectArray.h"

namespace ConfigDB::Json
{
class Store : public ConfigDB::Store
{
public:
	Store(Database& db, const String& name) : ConfigDB::Store(db, name)
	{
		load();
	}

	void* getObjectDataPtr(const ObjectInfo& object);

	template <typename T> T& getObjectData(const ObjectInfo& object)
	{
		return *reinterpret_cast<T*>(getObjectDataPtr(object));
	}

	bool commit() override
	{
		return save();
	}

	String getFilename() const
	{
		return getPath() + ".json";
	}

	/**
	 * @brief Resolve a path into the corresponding JSON object, creating it if required
	 */
	// JsonObject getJsonObject(const String& path);
	// JsonObjectConst getJsonObjectConst(const String& path) const;

	// JsonArray getJsonArray(const String& path);
	// JsonArrayConst getJsonArrayConst(const String& path) const;

	size_t printTo(Print& p) const override;

	template <class T> static size_t printObjectTo(T& obj, Format format, Print& p)
	{
		// switch(format) {
		// case Format::Compact:
		// 	return serializeJson(obj, p);
		// case Format::Pretty:
		// 	return serializeJsonPretty(obj, p);
		// }
		return 0;
	}

private:
	friend class Object;

	bool load();
	bool save();

	// StaticJsonDocument<1024> doc;
};

template <class ClassType> using StoreTemplate = ConfigDB::StoreTemplate<Store, ClassType>;

} // namespace ConfigDB::Json
