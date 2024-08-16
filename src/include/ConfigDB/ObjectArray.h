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

	Object getObject(unsigned index)
	{
		if(index > getObjectCount()) {
			return {};
		}
		return Object(*typeinfo().objinfo[0], *this, getObjectDataPtr(index));
	}

	void* getObjectDataPtr(unsigned index);

	unsigned getObjectCount() const
	{
		return id() ? getArray().getCount() : 0;
	}

	Object addNewObject()
	{
		return getObject(getObjectCount());
	}

	bool removeItem(unsigned index)
	{
		return id() && getArray().remove(index);
	}

	ArrayId id() const
	{
		return *static_cast<ArrayId*>(data);
	}

private:
	ArrayData& getArray();

	const ArrayData& getArray() const
	{
		// ArrayData will be created if it doesn't exist, but will be returned const to prevent updates
		return const_cast<ObjectArray*>(this)->getArray();
	}
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

	explicit ObjectArrayTemplate(Store& store) : ObjectArray(ClassType::typeinfo, store)
	{
	}

	ObjectArrayTemplate(Object& parent, void* data) : ObjectArray(ClassType::typeinfo, parent, data)
	{
	}

	Item getItem(unsigned index)
	{
		return Item(*this, index);
	}

	Item addItem()
	{
		return Item(*this, getObjectData(getObjectCount()));
	}

private:
	typename Item::Struct& getObjectData(unsigned index)
	{
		return *static_cast<typename Item::Struct*>(getObjectDataPtr(index));
	}
};

} // namespace ConfigDB
