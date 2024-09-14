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
		return Object(*this, 0, index);
	}

	Object getItem(unsigned index)
	{
		return getObject(index);
	}

	unsigned getObjectCount() const
	{
		return getItemCount();
	}

	template <typename Item = Object> Item addItem()
	{
		if(!this->writeCheck()) {
			return {};
		}
		auto& array = getArray();
		auto index = array.getCount();
		auto& itemType = getItemType();
		array.add(itemType.variant.object->defaultData);
		return Item(*this, 0, index);
	}

	template <typename Item = Object> Item insertItem(unsigned index)
	{
		if(!this->writeCheck()) {
			return {};
		}
		auto& itemType = getItemType();
		auto& array = getArray();
		array.insert(index, itemType.variant.object->defaultData);
		return Item(*this, 0, index);
	}

	const PropertyInfo& getItemType() const
	{
		return typeinfo().propinfo[0];
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
	using ObjectArray::ObjectArray;

	const ItemType operator[](unsigned index) const
	{
		return ItemType(*this, index);
	}
};

/**
 * @brief Used by code generator
 * @tparam UpdaterType
 * @tparam ClassType Contained class with type information
 * @tparam ItemType Updater item type
 */
template <class UpdaterType, class ClassType, class ItemType> class ObjectArrayUpdaterTemplate : public ClassType
{
public:
	using ClassType::ClassType;

	ItemType operator[](unsigned index)
	{
		return ItemType(*this, 0, index);
	}

	ItemType addItem()
	{
		return ObjectArray::addItem<ItemType>();
	}

	ItemType insertItem(unsigned index)
	{
		return ObjectArray::insertItem<ItemType>(index);
	}
};

} // namespace ConfigDB
