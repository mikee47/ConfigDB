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
#include "../Object.h"
#include "../Array.h"
#include <ArduinoJson.h>

namespace ConfigDB::Json
{
class Store : public ConfigDB::Store
{
public:
	Store(Database& db, const String& name) : ConfigDB::Store(db, name)
	{
		load();
	}

	bool commit() override
	{
		return save();
	}

	String getStringValue(const String& path, const String& key) const override
	{
		JsonVariantConst value = getJsonObjectConst(path)[key];
		if(value.isNull()) {
			return nullptr;
		}
		return value;
	}

	size_t printTo(Print& p) const override;

	size_t printObjectTo(const Object& object, Print& p) const override;

	size_t printArrayTo(const Array& array, Print& p) const override;

	String getFilename() const
	{
		return getPath() + ".json";
	}

	/**
	 * @brief Resolve a path into the corresponding JSON object, creating it if required
	 */
	JsonObject getJsonObject(const String& path);
	JsonObjectConst getJsonObjectConst(const String& path) const;

	JsonArray getJsonArray(const String& path);
	JsonArrayConst getJsonArrayConst(const String& path) const;

private:
	bool load();
	bool save();

	StaticJsonDocument<1024> doc;
};

template <class ClassType> using StoreTemplate = ConfigDB::StoreTemplate<Store, ClassType>;

template <class ClassType> class ObjectTemplate : public ConfigDB::ObjectTemplate<Store, ClassType>
{
public:
	ObjectTemplate(std::shared_ptr<Store> store, const String& path)
		: ConfigDB::ObjectTemplate<Store, ClassType>(store, path)
	{
		object = store->getJsonObject(path);
	}

	template <typename T> bool setValue(const String& key, const T& value)
	{
		return object[key].set(value);
	}

	template <typename T> T getValue(const String& key, const T& defaultValue = {}) const
	{
		return object[key] | defaultValue;
	}

	JsonObject object;
};

template <class ClassType> class ArrayTemplate : public ConfigDB::ArrayTemplate<Store, ClassType>
{
public:
	ArrayTemplate(std::shared_ptr<Store> store, const String& path)
		: ConfigDB::ArrayTemplate<Store, ClassType>(store, path)
	{
		array = store->getJsonArray(path);

		// String s = ::Json::serialize(array);
		// debug_i("%s(%s): %s", __FUNCTION__, path.c_str(), s.c_str());
	}

	template <class Item> Item addItemObject()
	{
		auto index = array.size();
		array.createNestedObject();
		return Item(*this, index);
	}

	template <class Item> Item getItemObject(unsigned index)
	{
		return Item(*this, index);
	}

	template <typename T> T getItem(unsigned index, const T& defaultValue = {}) const
	{
		return array[index] | defaultValue;
	}

	template <typename T> bool setItem(unsigned index, const T& value)
	{
		return array[index].set(value);
	}

	template <typename T> bool addItem(const T& value)
	{
		return array.add(value);
	}

	template <typename T> bool removeItem(unsigned index)
	{
		array.remove(index);
		return true;
	}

	JsonArray array;
};

template <class ArrayType, class ClassType> class ItemObjectTemplate : public ObjectTemplate<ClassType>
{
public:
	ItemObjectTemplate(ArrayTemplate<ArrayType>& array, unsigned index) :
		ObjectTemplate<ClassType>(std::static_pointer_cast<Store>(array.getStore()), nullptr),
		object(array.template array[index])
	{
	}

	template <typename T> bool setValue(const String& key, const T& value)
	{
		object[key] = value;
		return true;
	}

	template <typename T> T getValue(const String& key, const T& defaultValue = {}) const
	{
		return object[key] | defaultValue;
	}

private:
	JsonObject object;
};

} // namespace ConfigDB::Json
