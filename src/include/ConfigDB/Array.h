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

	template <typename T> typename std::enable_if<std::is_integral<T>::value, T>::type getItem(unsigned index) const
	{
		return *static_cast<const T*>(getArray()[index]);
	}

	template <typename T> typename std::enable_if<std::is_same<T, String>::value, T>::type getItem(unsigned index) const
	{
		return static_cast<const char*>(getArray()[index]);
	}

	template <typename T>
	typename std::enable_if<std::is_integral<T>::value, bool>::type setItem(unsigned index, T value)
	{
		return getArray().set(index, value);
	}

	bool setItem(unsigned index, const String& value)
	{
		return getArray().set(index, getStringId(value));
	}

	bool addNewItem(const char* value, size_t valueLength);

	template <typename T> typename std::enable_if<std::is_integral<T>::value, bool>::type addItem(T value)
	{
		return getArray().add(value);
	}

	bool addItem(const String& value)
	{
		return getArray().add(getStringId(value));
	}

	bool removeItem(unsigned index)
	{
		return getArray().remove(index);
	}

	unsigned getPropertyCount() const
	{
		return getArray().getCount();
	}

	Property getProperty(unsigned index);

	ArrayId id() const
	{
		return *static_cast<ArrayId*>(data);
	}

private:
	ArrayData& getArray();

	const ArrayData& getArray() const
	{
		// ArrayData will be created if it doesn't exist, but will be returned const to prevent updates
		return const_cast<Array*>(this)->getArray();
	}
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 */
template <class ClassType> class ArrayTemplate : public Array
{
public:
	explicit ArrayTemplate(Store& store) : Array(ClassType::typeinfo, store)
	{
	}

	ArrayTemplate(Object& parent, ArrayId* id) : Array(ClassType::typeinfo, parent, id)
	{
	}
};

} // namespace ConfigDB
