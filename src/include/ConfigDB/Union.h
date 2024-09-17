/**
 * ConfigDB/Union.h
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
 * @brief Variant object, can contain one of a selection of objects types
 */
class Union : public Object
{
public:
	using Tag = uint8_t;

	using Object::Object;

	/**
	 * @brief Get the current tag which identifies the stored type
	 */
	Tag getTag() const
	{
		return getPropertyData(0)->uint8;
	}

	/**
	 * @brief Set the current tag and reset content to object default
	 */
	void setTag(Tag tag)
	{
		assert(tag < typeinfo().objectCount);
		getPropertyData(0)->uint8 = tag;
		auto& prop = typeinfo().propinfo[tag];
		auto data = static_cast<uint8_t*>(getDataPtr());
		data += prop.offset;
		auto obj = prop.variant.object;
		if(obj->defaultData) {
			memcpy_P(data, obj->defaultData, obj->structSize);
		} else {
			memset(data, 0, obj->structSize);
		}
	}

	unsigned getObjectCount() const
	{
		return 1;
	}

	Object getObject([[maybe_unused]] unsigned index)
	{
		assert(index == 0);
		return Object(*this, getTag());
	}

	unsigned getPropertyCount() const
	{
		return 0;
	}

	PropertyConst getProperty(unsigned) const
	{
		return {};
	}

	Property getProperty(unsigned)
	{
		return {};
	}

	/**
	 * @brief Used by code generator to obtain a read-only reference to the contained object
	 */
	template <typename Item> const Item as(Tag tag) const
	{
		assert(getTag() == tag);
		if(getTag() != tag) {
			return {};
		}
		return Item(*parent, typeinfo().getObject(tag), dataRef + propinfo().offset);
	}

	/**
	 * @brief Used by code generator to obtain a writeable reference to the contained object
	 */
	template <typename Item> Item as(Tag tag)
	{
		assert(getTag() == tag);
		if(getTag() != tag) {
			return {};
		}
		return Item(*parent, typeinfo().getObject(tag), dataRef + propinfo().offset);
	}

	/**
	 * @brief Used by code generator to set the tag and return a direct reference to the contained object.
	 */
	template <typename Item> Item to(Tag tag)
	{
		setTag(tag);
		return Item(*parent, typeinfo().getObject(tag), dataRef + propinfo().offset);
	}
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator
 */
template <class ClassType> class UnionTemplate : public Union
{
public:
	using Union::Union;
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
