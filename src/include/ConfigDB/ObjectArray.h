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

#include "ArrayBase.h"

namespace ConfigDB
{
/**
 * @brief Base class to provide array of objects
 */
class ObjectArray : public ArrayBase
{
public:
	using ArrayBase::ArrayBase;

	Object getObject(unsigned index)
	{
		return Object(getItemType(), *this, index);
	}

	unsigned getObjectCount() const
	{
		return getItemCount();
	}

	Object addItem()
	{
		auto& itemType = getItemType();
		auto& array = getArray();
		auto ref = array.getCount();
		array.add(itemType.defaultData);
		return Object(itemType, *this, ref);
	}

	const ObjectInfo& getItemType() const
	{
		return *typeinfo().objinfo[0];
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
	explicit ObjectArrayTemplate(Store& store) : ObjectArray(ClassType::typeinfo, store)
	{
	}

	ObjectArrayTemplate(Object& parent, uint16_t dataRef) : ObjectArray(ClassType::typeinfo, parent, dataRef)
	{
	}

	const ItemType operator[](unsigned index) const
	{
		return ItemType(*this, index);
	}
};

/**
 * @brief Used by code generator
 * @tparam UpdaterType
 * @tparam ClassType Contained class with type information
 * @tparam ItemType
 */
template <class UpdaterType, class ClassType, class ItemType> class ObjectArrayUpdaterTemplate : public ClassType
{
public:
	using ClassType::ClassType;

	ItemType operator[](unsigned index)
	{
		return ItemType(*this, index);
	}

	ItemType addItem()
	{
		if(!this->writeCheck()) {
			return ItemType(*this, 0);
		}
		auto& array = this->getArray();
		auto index = array.getCount();
		array.add(ItemType::typeinfo.defaultData);
		return ItemType(*this, index);
	}

	ItemType insertItem(unsigned index)
	{
		if(!this->writeCheck()) {
			return Item(*this, 0);
		}
		this->getArray().insert(index, ItemType::typeinfo.defaultData);
		return ItemType(*this, index);
	}
};

} // namespace ConfigDB
