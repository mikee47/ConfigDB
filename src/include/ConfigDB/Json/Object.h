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

class Object : public ConfigDB::Object
{
public:
	Object() = default;

	Object(JsonObject obj) : ConfigDB::Object(), object(obj)
	{
	}

	Object(Object& parent, const String& name) : ConfigDB::Object(parent), object(parent.object[name])
	{
	}

	Object(Array& parent, unsigned index);

	explicit operator bool() const
	{
		return !object.isNull();
	}

	String getStringValue(const String& key) const override;

	String getStringValue(unsigned index) const override;

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
	friend class RootObject;

	JsonObject object;
};

template <class ClassType> class ObjectTemplate : public Object
{
public:
	using Object::Object;
};

} // namespace ConfigDB::Json
