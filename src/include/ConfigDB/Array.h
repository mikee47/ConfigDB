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

namespace ConfigDB
{
/**
 * @brief Base class to provide array of properties
 */
class Array : public Object
{
public:
	Array(Store& store, const ObjectInfo& typeinfo);

	Array(Object& parent, ArrayId& id) : Object(parent), id(id)
	{
	}

	template <typename T> typename std::enable_if<std::is_integral<T>::value, T>::type getItem(unsigned index) const
	{
		return *static_cast<const T*>(getItemPtr(index));
	}

	template <typename T> typename std::enable_if<std::is_same<T, String>::value, T>::type getItem(unsigned index) const
	{
		return static_cast<const char*>(getItemPtr(index));
	}

	template <typename T>
	typename std::enable_if<std::is_integral<T>::value, bool>::type setItem(unsigned index, T value)
	{
		return setItemPtr(index, &value);
	}

	bool setItem(unsigned index, const String& value)
	{
		auto id = addString(value);
		return setItemPtr(index, &id);
	}

	template <typename T> typename std::enable_if<std::is_integral<T>::value, bool>::type addItem(T value)
	{
		return addItemPtr(&value);
	}

	bool addItem(const String& value)
	{
		auto id = addString(value);
		return addItemPtr(&id);
	}

	bool removeItem(unsigned index);

	std::unique_ptr<Object> getObject(unsigned) override
	{
		return nullptr;
	}

	unsigned getPropertyCount() const override;

	Property getProperty(unsigned index) override
	{
		if(index >= getPropertyCount()) {
			return {};
		}
		// Property info contains exactly one element
		auto& typeinfo = getTypeinfo();
		assert(typeinfo.propertyCount == 1);
		return {*this, typeinfo.propinfo[0], getItemPtr(index)};
	}

	void* getData() override
	{
		return &id;
	}

private:
	void* getItemPtr(unsigned index);

	const void* getItemPtr(unsigned index) const
	{
		return const_cast<Array*>(this)->getItemPtr(index);
	}

	bool setItemPtr(unsigned index, const void* value);
	bool addItemPtr(const void* value);

	ArrayId& id;
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 */
template <class ClassType> class ArrayTemplate : public Array
{
public:
	using Array::Array;

	const ObjectInfo& getTypeinfo() const override
	{
		return static_cast<const ClassType*>(this)->typeinfo;
	}
};

} // namespace ConfigDB
