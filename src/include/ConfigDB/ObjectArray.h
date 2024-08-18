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
#include "Pool.h"

namespace ConfigDB
{
/**
 * @brief Base class to provide array of objects
 */
class ObjectArray : public Object
{
public:
	using Object::Object;

	Object getObject(unsigned index);

	unsigned getObjectCount() const
	{
		return getId() ? getArray().getCount() : 0;
	}

	Object addItem()
	{
		auto& itemType = getItemType();
		auto& array = getArray();
		auto ref = array.getCount();
		array.add(itemType);
		return Object(itemType, this, ref);
	}

	bool removeItem(unsigned index)
	{
		return getId() && getArray().remove(index);
	}

	ArrayId& getId()
	{
		return *static_cast<ArrayId*>(getData());
	}

	ArrayId getId() const
	{
		return const_cast<ObjectArray*>(this)->getId();
	}

	void* getItemData(ArrayId ref)
	{
		return getArray()[ref];
	}

protected:
	const ObjectInfo& getItemType() const
	{
		return *typeinfo().objinfo[0];
	}

	ArrayData& getArray();

	const ArrayData& getArray() const
	{
		// ArrayData will be created if it doesn't exist, but will be returned const to prevent updates
		return const_cast<ObjectArray*>(this)->getArray();
	}
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete class type
 * @tparam ItemType Array item class type
 */
template <class ClassType, class ItemType> class ObjectArrayTemplate : public ObjectArray
{
public:
	using Item = ItemType;

	explicit ObjectArrayTemplate(Store& store) : ObjectArray(ClassType::typeinfo, store)
	{
	}

	ObjectArrayTemplate(Object& parent, uint16_t dataRef) : ObjectArray(ClassType::typeinfo, &parent, dataRef)
	{
	}

	Item getItem(unsigned index)
	{
		return Item(*this, index);
	}

	Item operator[](unsigned index)
	{
		return getItem(index);
	}

	Item addItem()
	{
		auto& array = getArray();
		uint16_t ref = array.getCount();
		array.add(Item::typeinfo);
		return Item(*this, ref);
	}
};

} // namespace ConfigDB
