/**
 * ConfigDB/Json/Object.h
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

#include "../Object.h"
#include <ArduinoJson.h>

namespace ConfigDB::Json
{
class Array;
class ObjectArray;
class Store;

class Object : public ConfigDB::Object
{
public:
	using ConfigDB::Object::Object;

	explicit Object(Store& store, const String& path);

	Object(Object& parent, JsonObject obj) : ConfigDB::Object(parent), object(obj)
	{
	}

	Object(Object& parent, const String& name) : Object(parent, get(parent, name))
	{
	}

	Object(ObjectArray& parent, JsonObject obj);

	static JsonObject get(Object& parent, const String& name)
	{
		JsonObject obj = parent.object[name];
		if(obj.isNull()) {
			obj = parent.object.createNestedObject(name);
		}
		return obj;
	}

	void init(Json::Object& parent, const String& name)
	{
		this->parent = &parent;
		object = get(parent, name);
	}

	explicit operator bool() const
	{
		return !object.isNull();
	}

	String getStringValue(const String& key) const override
	{
		return object[key].as<const char*>();
	}

	String getStringValue(unsigned index) const override
	{
		return nullptr;
	}

	// [CODEGEN]
	// unsigned getPropertyCount() const override
	// Property getProperty(unsigned index) override

	template <typename T> bool setValue(const String& key, const T& value)
	{
		return object[key].set(value);
	}

	template <typename T> T getValue(const String& key, const T& defaultValue = {}) const
	{
		return object[key] | defaultValue;
	}

	size_t printTo(Print& p) const override;

private:
	friend class Store;
	friend class Array;
	friend class ObjectArray;
	friend class RootObject;

	JsonObject object;
};

template <class ClassType> class ObjectTemplate : public Object
{
public:
	using Object::Object;

	ObjectTemplate(Object& parent) : Object(parent)
	{
	}
};

} // namespace ConfigDB::Json
