/**
 * ConfigDB/ArrayBase.h
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
class Union : public Object
{
public:
	using Tag = uint8_t;

	using Object::Object;

	Tag getTag() const
	{
		return getPropertyData(0)->uint8;
	}

	void setTag(Tag tag)
	{
		assert(tag < typeinfo().objectCount);
		getPropertyData(0)->uint8 = tag;
		auto& prop = typeinfo().propinfo[tag];
		auto data = static_cast<uint8_t*>(getDataPtr());
		data += sizeof(Tag);
		if(prop.object->defaultData) {
			memcpy_P(data, prop.object->defaultData, prop.object->structSize);
		} else {
			memset(data, 0, prop.object->structSize);
		}
	}

	Object getObject(unsigned index)
	{
		assert(index == 0);
		auto tag = getTag();
		auto& prop = typeinfo().getObject(tag);
		return prop ? Object(prop, *this, sizeof(Tag)) : Object();
	}
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator
 */
template <class ClassType> class UnionTemplate : public Union
{
public:
	UnionTemplate() : Union(ClassType::typeinfo)
	{
	}

	explicit UnionTemplate(Store& store) : Union(ClassType::typeinfo, store)
	{
	}

	UnionTemplate(Object& parent, uint16_t dataRef) : Union(ClassType::typeinfo, parent, dataRef)
	{
	}

	UnionTemplate(const Object& parent, uint16_t dataRef)
		: Union(ClassType::typeinfo, const_cast<Object&>(parent), dataRef)
	{
	}
};

/**
 * @brief Used by code generator
 * @tparam UpdaterType
 * @tparam ClassType Contained class with type information
 */
template <class UpdaterType, class ClassType> class UnionUpdaterTemplate : public ClassType
{
public:
	using ClassType::ClassType;

	explicit operator bool() const
	{
		return Object::operator bool() && this->isWriteable();
	}
};

} // namespace ConfigDB
