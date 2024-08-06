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
	Array() = default;

	Array(Object& parent) : Object(parent)
	{
	}

	String getStoredValue(const String&) const override
	{
		return nullptr;
	}

	std::unique_ptr<Object> getObject(unsigned) override
	{
		return nullptr;
	}

	Property getProperty(unsigned index) override
	{
		// Property info contains exactly one element
		auto propinfo = getTypeinfo().propinfo;
		if(index < getPropertyCount() && propinfo) {
			return {*this, index, *propinfo->data()};
		}
		return {};
	}
};

/**
 * @brief Used by store implemention to create specific template for `Array`
 * @tparam BaseType The store's base `Array` class
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 */
template <class BaseType, class ClassType> class ArrayTemplate : public BaseType
{
public:
	using BaseType::BaseType;

	const ObjectInfo& getTypeinfo() const override
	{
		return static_cast<const ClassType*>(this)->typeinfo;
	}
};

} // namespace ConfigDB
