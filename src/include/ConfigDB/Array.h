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

#include "Object.h"
#include "Pool.h"

namespace ConfigDB
{
/**
 * @brief Base class to provide array of properties
 */
class Array : public Object
{
public:
	using Object::Object;

	unsigned getPropertyCount() const
	{
		return getId() ? getArray().getCount() : 0;
	}

	unsigned getItemCount() const
	{
		return getPropertyCount();
	}

	Property getProperty(unsigned index);

	void addNewItem(const char* value, size_t valueLength);

	bool removeItem(unsigned index)
	{
		return getArray().remove(index);
	}

	ArrayId& getId()
	{
		return *static_cast<ArrayId*>(getData());
	}

	ArrayId getId() const
	{
		return *static_cast<const ArrayId*>(getData());
	}

	void* getItemData(ArrayId ref)
	{
		return getArray()[ref];
	}

	const PropertyInfo& getItemType() const
	{
		assert(typeinfo().propertyCount == 1);
		return typeinfo().propinfo[0];
	}

protected:
	ArrayData& getArray();
	const ArrayData& getArray() const;
};

/**
 * @brief Used by code generator for integral-typed arrays
 * @tparam ClassType Concrete type provided by code generator
 * @tparam ItemType Type of item
 */
template <class ClassType, typename ItemType> class ArrayTemplate : public Array
{
public:
	struct ItemRef {
		Array& array;
		unsigned index;

		operator ItemType() const
		{
			return static_cast<ClassType&>(array).getItem(index);
		}

		ItemRef& operator=(const String& value)
		{
			static_cast<ClassType&>(array).setItem(index, value);
			return *this;
		}
	};

	explicit ArrayTemplate(Store& store) : Array(ClassType::typeinfo, store)
	{
	}

	ArrayTemplate(Object& parent, uint16_t dataRef) : Array(ClassType::typeinfo, &parent, dataRef)
	{
	}

	ItemType getItem(unsigned index) const
	{
		return *static_cast<const ItemType*>(getArray()[index]);
	}

	void setItem(unsigned index, ItemType value)
	{
		*static_cast<ItemType*>(getArray()[index]) = value;
	}

	void addItem(ItemType value)
	{
		getArray().add(value);
	}

	ItemRef operator[](unsigned index)
	{
		return {*this, index};
	}

	const ItemType operator[](unsigned index) const
	{
		return this->getItem(index);
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
		return this->getString(id);
	}

	void setItem(unsigned index, const String& value)
	{
		*static_cast<StringId*>(this->getArray()[index]) = this->getStringId(value);
	}

	void addItem(const String& value)
	{
		this->getArray().add(this->getStringId(value));
	}
};

} // namespace ConfigDB
