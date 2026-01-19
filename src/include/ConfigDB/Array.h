/****
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
#include "ArrayIterator.h"

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
	friend class Object;

	void loadDefaults();

	StringId getStringId(const String& value)
	{
		return ArrayBase::getStringId(getItemType(), value);
	}

	template <typename T> void addItem(const T& value)
	{
		PropertyData dst{};
		dst.setValue(getItemType(), value);
		this->getArray().add(&dst);
	}

	template <typename T> void insertItem(unsigned index, const T& value)
	{
		PropertyData dst{};
		dst.setValue(getItemType(), value);
		this->getArray().insert(index, &dst);
	}

	template <typename T> void setItem(unsigned index, const T& value)
	{
		auto dst = static_cast<PropertyData*>(ArrayBase::getItem(index));
		dst->setValue(getItemType(), value);
	}

	int indexOf(const void* value) const
	{
		auto itemSize = getItemType().getSize();
		auto& array = getArray();
		for(unsigned i = 0; i < array.getCount(); ++i) {
			if(memcmp(array[i], value, itemSize) == 0) {
				return i;
			}
		}
		return -1;
	}

private:
	Property makeProperty(void* data)
	{
		return {getStore(), getItemType(), static_cast<PropertyData*>(data), nullptr};
	}

	PropertyConst makeProperty(const void* data) const
	{
		return {getStore(), getItemType(), static_cast<const PropertyData*>(data)};
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
	using Iterator = const ArrayIterator<const ArrayTemplate, const ItemType>;

	using Array::Array;

	ItemType getItem(unsigned index) const
	{
		return *static_cast<const ItemType*>(getArray()[index]);
	}

	const ItemType operator[](unsigned index) const
	{
		return static_cast<const ClassType*>(this)->getItem(index);
	}

	int indexOf(ItemType item) const
	{
		return Array::indexOf(&item);
	}

	bool contains(ItemType item) const
	{
		return indexOf(item) >= 0;
	}

	Iterator begin() const
	{
		return Iterator(*this, 0);
	}

	Iterator end() const
	{
		return Iterator(*this, this->getItemCount());
	}
};

/**
 * @brief Used by code generator for integral-typed arrays
 * @tparam UpdaterType
 * @tparam ClassType Contained class with type information
 * @tparam ItemType Updater item type for returning
 * @tparam ItemSetType Updater item type for set methods
 * @tparam ItemCastType Integral type for setting item
 */
template <class UpdaterType, class ClassType, typename ItemType, typename ItemSetType, typename ItemCastType>
class ArrayUpdaterTemplate : public ClassType
{
public:
	struct ItemRef {
		Array& array;
		unsigned index;

		operator ItemType() const
		{
			return static_cast<UpdaterType&>(array).getItem(index);
		}

		ItemRef& operator=(const ItemSetType& value)
		{
			static_cast<UpdaterType&>(array).setItem(index, ItemCastType(value));
			return *this;
		}
	};

	using Iterator = ArrayIterator<ArrayUpdaterTemplate, ItemRef>;

	using ClassType::ClassType;

	void setItem(unsigned index, const ItemSetType& value)
	{
		Array::setItem(index, ItemCastType(value));
	}

	void addItem(const ItemSetType& value)
	{
		Array::addItem(ItemCastType(value));
	}

	void insertItem(unsigned index, const ItemSetType& value)
	{
		Array::insertItem(index, ItemCastType(value));
	}

	ItemRef operator[](unsigned index)
	{
		return {*this, index};
	}

	Iterator begin()
	{
		return Iterator(*this, 0);
	}

	Iterator end()
	{
		return Iterator(*this, this->getItemCount());
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

	int indexOf(const String& item) const
	{
		int stringId = this->findStringId(item.c_str(), item.length());
		return Array::indexOf(&stringId);
	}

	bool contains(const String& item) const
	{
		return indexOf(item) >= 0;
	}
};

/**
 * @brief Used by code generator for String-typed arrays
 * @tparam UpdaterType
 * @tparam ClassType Contained class with type information
 * @tparam ItemType always String, but could support string-like objects if we want
 */
template <class UpdaterType, class ClassType, typename ItemType, typename ItemSetType, typename ItemCastType>
class StringArrayUpdaterTemplate
	: public ArrayUpdaterTemplate<UpdaterType, ClassType, ItemType, ItemSetType, ItemCastType>
{
public:
	using ArrayUpdaterTemplate<UpdaterType, ClassType, ItemType, ItemSetType, ItemCastType>::ArrayUpdaterTemplate;

	void setItem(unsigned index, const ItemType& value)
	{
		auto stringId = this->getStringId(value);
		Array::setItem(index, stringId);
	}

	void addItem(const ItemType& value)
	{
		auto stringId = this->getStringId(value);
		Array::addItem(stringId);
	}

	void insertItem(unsigned index, const ItemType& value)
	{
		auto stringId = this->getStringId(value);
		Array::insertItem(index, stringId);
	}
};

} // namespace ConfigDB
