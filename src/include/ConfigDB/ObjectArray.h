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
	// ObjectArray() = default;

	ObjectArray(Store& store, const ObjectInfo& typeinfo);

	ObjectArray(Object& parent, ArrayId& id) : Object(parent), id(id)
	{
	}

	void* getObjectDataPtr(unsigned index);

	template <class Item> Item& getObjectData(unsigned index)
	{
		return *reinterpret_cast<Item*>(getObjectDataPtr(index));
	}

	bool removeItem(unsigned index)
	{
		// array.remove(index);
		return true;
	}

	unsigned getObjectCount() const override;

	unsigned getPropertyCount() const override
	{
		return 0;
	}

	Property getProperty(unsigned)
	{
		return {};
	}

private:
	ArrayId& id;
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 * @tparam Item Concrete type for array item provided by code generator
 */
template <class ClassType, class ItemType> class ObjectArrayTemplate : public ObjectArray
{
public:
	using Item = ItemType;

	using ObjectArray::ObjectArray;

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
		return Item(*this, this->getObjectCount());
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
