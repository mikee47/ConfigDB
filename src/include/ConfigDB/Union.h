/****
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
 * @brief Variant object, can contain one of a selection of object types
 * A union contains one private property, the tag, and one or more object types.
 * Only one of these objects is stored, accessed at object index 0.
 */
class Union : public Object
{
public:
	/**
	 * @brief A zero-based index which identifies the stored object or property type
	 */
	using Tag = uint8_t;

	using Object::Object;

	/**
	 * @brief Get the current tag which identifies the stored type
	 */
	Tag getTag() const
	{
		auto ptr = static_cast<const uint8_t*>(getDataPtr());
		ptr += typeinfo().dataSize - 1;
		return *ptr;
	}

	/**
	 * @brief Get string value for current tag
	 */
	String getTagString() const
	{
		return propinfo().variant.object->propinfo[getTag()].name;
	}

	bool tagIsObject(Tag tag) const
	{
		return tag < typeinfo().objectCount;
	}

	bool tagIsObject() const
	{
		return tagIsObject(getTag());
	}

	/**
	 * @brief Set the current tag and reset content to object default
	 */
	void setTag(Tag tag);

	/**
	 * @brief Reset tag to default and clear whatever object that corresponds to
	 */
	void clear()
	{
		setTag(getDefaultTag());
	}

	Tag getDefaultTag() const
	{
		auto& ti = typeinfo();
		auto ptr = static_cast<const uint8_t*>(ti.defaultData);
		if(!ptr) {
			assert(false);
			return 0;
		}
		unsigned n = ti.objectCount + ti.propertyCount;
		for(auto prop = ti.propinfo; n--; ++prop) {
			if(prop->type == PropertyType::Object) {
				ptr += prop->variant.object->dataSize;
			} else {
				ptr += prop->getSize();
			}
		}
		return pgm_read_byte(ptr);
	}

	unsigned getObjectCount() const
	{
		return tagIsObject() ? 1 : 0;
	}

	Object getObject(unsigned index)
	{
		if(index != 0) {
			assert(false);
			return {};
		}
		return {*this, getTag()};
	}

	unsigned getPropertyCount() const
	{
		return tagIsObject() ? 0 : 1;
	}

	PropertyConst getProperty(unsigned index) const
	{
		if(index != 0) {
			assert(false);
			return {};
		}
		return {*this, getTag() - typeinfo().objectCount};
	}

	Property getProperty(unsigned index)
	{
		if(index != 0) {
			assert(false);
			return {};
		}
		return {*this, getTag() - typeinfo().objectCount};
	}

	/**
	 * @brief Used by code generator to obtain a read-only reference to the contained object
	 * @tparam Item The item type
	 * @param tag The tag for the given item type
	 */
	template <typename Item> const Item as(Tag tag) const
	{
		if(getTag() != tag) {
			assert(false);
			return {};
		}
		return Item(*parent, typeinfo().getObject(tag), dataRef + propinfo().offset);
	}

	/**
	 * @brief Used by code generator to obtain a writeable reference to the contained object
	 * @tparam Item The item type
	 * @param tag The tag for the given item type
	 */
	template <typename Item> Item as(Tag tag)
	{
		if(getTag() != tag) {
			assert(false);
			return {};
		}
		return Item(*parent, typeinfo().getObject(tag), dataRef + propinfo().offset);
	}

	/**
	 * @brief Used by code generator to set the tag and return a direct reference to the contained object.
	 * @tparam Item The item type
	 * @param tag The tag for the desired item type
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
