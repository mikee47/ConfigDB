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
class Array : public Object
{
public:
	Array() = default;

	Array(Object& parent) : Object(parent)
	{
	}

	unsigned getObjectCount() const override
	{
		return 0;
	}

	std::unique_ptr<Object> getObject(unsigned index) override
	{
		return nullptr;
	}

	Property getProperty(unsigned index) override
	{
		auto propcount = getPropertyCount();
		auto propinfo = getTypeinfo().propinfo;
		if(index >= propcount || !propinfo) {
			return {};
		}
		return {*this, index, (*propinfo)[0]};
	}
};

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
