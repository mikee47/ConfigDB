/**
 * ConfigDB/ObjectArray.h
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

namespace ConfigDB
{
/**
 * @brief Base class to provide array of objects
 */
class ObjectArray : public Object
{
public:
	ObjectArray() = default;

	ObjectArray(Object& parent) : Object(parent)
	{
	}

	String getStoredValue(const String&) const override
	{
		return nullptr;
	}

	String getStoredArrayValue(unsigned) const override
	{
		return nullptr;
	}

	unsigned getPropertyCount() const override
	{
		return 0;
	}

	Property getProperty(unsigned)
	{
		return {};
	}
};

/**
 * @brief Used by store implemention to create specific template for `ObjectArray`
 * @tparam BaseType The store's base `Array` class
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 * @tparam Item Concrete type for array item provided by code generator
 */
template <class BaseType, class ClassType, class ItemType> class ObjectArrayTemplate : public BaseType
{
public:
	using Item = ItemType;

	using BaseType::BaseType;

	const ObjectInfo& getTypeinfo() const override
	{
		return static_cast<const ClassType*>(this)->typeinfo;
	}

	Item getItem(unsigned index)
	{
		return Item(*this, index);
	}

	Item addItem()
	{
		return Item(*this);
	}

	std::unique_ptr<ConfigDB::Object> getObject(unsigned index) override
	{
		if(index >= this->getObjectCount()) {
			return nullptr;
		}
		return std::make_unique<Item>(*this, index);
	}
};

} // namespace ConfigDB
