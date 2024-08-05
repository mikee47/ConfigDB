/**
 * ConfigDB/Json/ObjectArray.h
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

#include "../ObjectArray.h"
#include "Object.h"

namespace ConfigDB::Json
{
class ObjectArray : public ConfigDB::ObjectArray
{
public:
	using ConfigDB::ObjectArray::ObjectArray;

	explicit ObjectArray(Store& store, const String& path);

	ObjectArray(Json::Object& parent, const String& name) : ObjectArray(parent)
	{
		array = parent.object[name];
		if(array.isNull()) {
			array = parent.object.createNestedArray(name);
		}
	}

	String getStringValue(const String& key) const override
	{
		return nullptr;
	}

	String getStringValue(unsigned index) const override
	{
		return nullptr;
	}

	explicit operator bool() const
	{
		return !array.isNull();
	}

	unsigned getObjectCount() const override
	{
		return array.size();
	}

	size_t printTo(Print& p) const override;

	bool removeItem(unsigned index)
	{
		array.remove(index);
		return true;
	}

protected:
	friend class Json::Object;

	JsonArray array;
};

template <class ClassType, class Item> class ObjectArrayTemplate : public ObjectArray
{
public:
	using ObjectArray::ObjectArray;

	const Typeinfo& getTypeinfo() const override
	{
		return static_cast<const ClassType*>(this)->typeinfo;
	}

	using ObjectArray::getObject;

	std::unique_ptr<ConfigDB::Object> getObject(unsigned index) override
	{
		if(index >= array.size()) {
			return nullptr;
		}
		return std::make_unique<Item>(*this, index);
	}

	Item getItem(unsigned index)
	{
		return Item(*this, array[index]);
	}

	bool setItem(unsigned index, const Item& value)
	{
		if(index >= array.size()) {
			return false;
		}
		return array[index].set(value.object);
	}

	Item addItem()
	{
		return Item(*this);
	}
};

} // namespace ConfigDB::Json
