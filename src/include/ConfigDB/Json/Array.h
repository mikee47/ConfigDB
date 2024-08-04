/**
 * ConfigDB/Json/Array.h
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

#include "../Array.h"
#include "Object.h"

namespace ConfigDB::Json
{
class Array : public ConfigDB::Array
{
public:
	using ConfigDB::Array::Array;

	explicit Array(Store& store, const String& path);

	Array(Json::Object& parent, JsonArray array) : ConfigDB::Array(parent), array(array)
	{
	}

	Array(Json::Object& parent, const String& name) : Array(parent, get(parent, name))
	{
	}

	Array(Array& parent, unsigned index) : ConfigDB::Array(parent)
	{
		array = parent.array[index];
	}

	static JsonArray get(Json::Object& parent, const String& name)
	{
		JsonArray arr = parent.object[name];
		if(arr.isNull()) {
			arr = parent.object.createNestedArray(name);
		}
		return arr;
	}

	void init(Json::Object& parent, const String& name)
	{
		this->parent = &parent;
		array = get(parent, name);
	}

	String getStringValue(const String& key) const override
	{
		return nullptr;
	}

	String getStringValue(unsigned index) const override
	{
		return array[index].as<const char*>();
	}

	explicit operator bool() const
	{
		return !array.isNull();
	}

	unsigned getPropertyCount() const override
	{
		return array.size();
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

	bool removeItem(unsigned index)
	{
		array.remove(index);
		return true;
	}

	size_t printTo(Print& p) const override;

protected:
	Property getArrayProperty(unsigned index, Property::Type type, const FlashString* defaultValue)
	{
		if(index >= array.size()) {
			return {};
		}
		return Property(*this, index, type, defaultValue);
	}

private:
	friend class Json::Object;

	JsonArray array;
};

template <class ClassType> class ArrayTemplate : public Array
{
public:
	using Array::Array;
};

} // namespace ConfigDB::Json
