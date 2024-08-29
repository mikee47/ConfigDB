/**
 * ConfigDB/Array.h
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
 * @brief Base class to provide array of properties
 */
class Array : public ArrayBase
{
public:
	using ArrayBase::ArrayBase;

	unsigned getPropertyCount() const
	{
		return getItemCount();
	}

	Property getProperty(unsigned index)
	{
		return makeProperty(getArray()[index]);
	}

	Property addItem()
	{
		return makeProperty(getArray().add());
	}

	Property insertItem(unsigned index)
	{
		return makeProperty(getArray().insert(index));
	}

	PropertyConst getProperty(unsigned index) const
	{
		return makeProperty(getArray()[index]);
	}

	const PropertyInfo& getItemType() const
	{
		assert(typeinfo().propertyCount == 1);
		return typeinfo().propinfo[0];
	}

protected:
	StringId getStringId(const String& value)
	{
		return ArrayBase::getStringId(getItemType(), value);
	}

private:
	Property makeProperty(void* data)
	{
		return {getStore(), getItemType(), static_cast<PropertyData*>(data), nullptr};
	}

	PropertyConst makeProperty(const void* data) const
	{
		return {getStore(), getItemType(), static_cast<const PropertyData*>(data), nullptr};
	}
};

/**
 * @brief Used by code generator for integral-typed arrays
 * @tparam ClassType Concrete type provided by code generator
 * @tparam ItemType Type of item
 */
template <class ClassType, typename ItemType> class ArrayTemplate : public Array
{
public:
	explicit ArrayTemplate(Store& store) : Array(ClassType::typeinfo, store)
	{
	}

	ArrayTemplate(Object& parent, uint16_t dataRef) : Array(ClassType::typeinfo, parent, dataRef)
	{
	}

	ItemType getItem(unsigned index) const
	{
		return *static_cast<const ItemType*>(getArray()[index]);
	}

	const ItemType operator[](unsigned index) const
	{
		return static_cast<const ClassType*>(this)->getItem(index);
	}
};

/**
 * @brief Used by code generator for integral-typed arrays
 * @tparam UpdaterType
 * @tparam ClassType Contained class with type information
 * @tparam ItemType Updater item type
 */
template <class UpdaterType, class ClassType, typename ItemType> class ArrayUpdaterTemplate : public ClassType
{
public:
	struct ItemRef {
		Array& array;
		unsigned index;

		operator ItemType() const
		{
			return static_cast<UpdaterType&>(array).getItem(index);
		}

		ItemRef& operator=(const ItemType& value)
		{
			static_cast<UpdaterType&>(array).setItem(index, value);
			return *this;
		}
	};

	using ClassType::ClassType;

	void setItem(unsigned index, ItemType value)
	{
		*static_cast<ItemType*>(ArrayBase::getItem(index)) = value;
	}

	void addItem(ItemType value)
	{
		this->getArray().add(&value);
	}

	void insertItem(unsigned index, ItemType value)
	{
		this->getArray().insert(index, &value);
	}

	ItemRef operator[](unsigned index)
	{
		return {*this, index};
	}
};

/**
 * @brief Used by code generator for String-typed arrays
 * @tparam ClassType Concrete type provided by code generator
 * @tparam ItemType Type of item, not used (always String)
 */
template <class ClassType, typename ItemType> class StringArrayTemplate : public ArrayTemplate<ClassType, ItemType>
{
public:
	using ArrayTemplate<ClassType, ItemType>::ArrayTemplate;

	String getItem(unsigned index) const
	{
		auto id = *static_cast<const StringId*>(this->getArray()[index]);
		return this->getPropertyString(0, id);
	}
};

/**
 * @brief Used by code generator for String-typed arrays
 * @tparam UpdaterType
 * @tparam ClassType Contained class with type information
 * @tparam ItemType always String, but could support string-like objects if we want
 */
template <class UpdaterType, class ClassType, typename ItemType>
class StringArrayUpdaterTemplate : public ArrayUpdaterTemplate<UpdaterType, ClassType, ItemType>
{
public:
	using ArrayUpdaterTemplate<UpdaterType, ClassType, ItemType>::ArrayUpdaterTemplate;

	void setItem(unsigned index, const ItemType& value)
	{
		*static_cast<StringId*>(ArrayBase::getItem(index)) = this->getStringId(value);
	}

	void addItem(const ItemType& value)
	{
		auto stringId = this->getStringId(value);
		this->getArray().add(&stringId);
	}

	void insertItem(unsigned index, const ItemType& value)
	{
		auto stringId = this->getStringId(value);
		this->getArray().insert(index, &stringId);
	}
};

} // namespace ConfigDB
