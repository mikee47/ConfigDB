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

#include "Object.h"

namespace ConfigDB::Json
{
class ObjectArray : public ConfigDB::ObjectArray
{
public:
	ObjectArray(Object& parent, const String& name) : ConfigDB::ObjectArray(parent)
	{
		array = parent.object[name];
	}

	ObjectArray(Object& parent, unsigned index) : ConfigDB::ObjectArray(parent)
	{
		array = parent.object[index];
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

private:
	JsonArray array;
};

template <class ClassType> class ObjectArrayTemplate : public ObjectArray
{
public:
	using ObjectArray::ObjectArray;

	std::unique_ptr<ConfigDB::Object> getObject(unsigned index) override
	{
		auto item = static_cast<ClassType*>(this)->getItem(index);
		if(item) {
			return std::make_unique<decltype(item)>(item);
		}
		return nullptr;
	}

	template <class Item> Item addItem()
	{
		auto index = array.size();
		array.createNestedObject();
		return Item(*this, index);
	}

	template <class Item> Item getItem(unsigned index)
	{
		return Item(*this, index);
	}

	bool removeItem(unsigned index)
	{
		array.remove(index);
		return true;
	}

	JsonArray array;
};

} // namespace ConfigDB::Json
