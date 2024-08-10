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

namespace ConfigDB::Json
{
class Array;
class ObjectArray;
class Store;

class Object : public ConfigDB::Object
{
public:
	using ConfigDB::Object::Object;

	explicit Object(Object& parent) : ConfigDB::Object(parent)
	{
		// NB. Name, path, etc. all in typeinfo
		// object = parent.object[name];
		// if(object.isNull()) {
		// 	object = parent.object.createNestedObject(name);
		// }
	}

	// Object(ObjectArray& parent, unsigned index);

	explicit Object(ObjectArray& parent);

	explicit operator bool() const
	{
		return object != 0;
	}

	String getPropertyValue(unsigned propIndex) const
	{
		return nullptr;
	}

	bool setPropertyValue(unsigned propIndex, const String& value) const
	{
		return false;
	}

	String getStoredValue(const String& key) const override
	{
		return nullptr;
		// return object[key].as<const char*>();
	}

	template <typename T> bool setValue(const PropertyInfo& prop, const T& value)
	{
		return false;
		// return object[prop.getName()].set(value);
	}

	template <typename T> T getValue(const PropertyInfo& prop) const
	{
		return {};
		// return object[prop.getName()] | defaultValue;
	}

	size_t printTo(Print& p) const override;

private:
	friend class Store;
	friend class Array;
	friend class ObjectArray;
	friend class RootObject;

	ObjectId object;
};

template <class ClassType> using ObjectTemplate = ConfigDB::ObjectTemplate<Object, ClassType>;

} // namespace ConfigDB::Json
