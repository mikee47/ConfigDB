/**
 * ConfigDB/Property.h
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

#include <WString.h>
#include <FlashString/Array.hpp>
#include <FlashString/Vector.hpp>
#include <memory>

#define CONFIGDB_PROPERTY_TYPE_MAP(XX)                                                                                 \
	XX(String)                                                                                                         \
	XX(Integer)                                                                                                        \
	XX(Boolean)

namespace ConfigDB
{
class Object;

enum class PropertyType {
#define XX(name) name,
	CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
};

struct PropertyInfo {
	const FlashString* name;
	const FlashString* defaultValue;
	PropertyType type;
};

/**
 * @brief Manages a key/value pair stored in an object
 */
class Property
{
public:
	Property() = default;

	using Type = PropertyType;

	/**
	 * @brief Property accessed by key
	 */
	Property(Object& object, const PropertyInfo& info) : object(&object), info(info)
	{
	}

	/**
	 * @brief Property accessed by index
	 */
	Property(Object& object, unsigned index, const PropertyInfo& info) : object(&object), info(info), index(index)
	{
	}

	String getName() const
	{
		return info.name ? String(*info.name) : nullptr;
	}

	unsigned getIndex() const
	{
		return index;
	}

	/**
	 * @brief Check if this property is accessed by index
	 * @retval bool true if this property is an array member, false if it's a named object property
	 */
	bool isIndexed() const
	{
		return info.name != nullptr;
	}

	String getDefaultStringValue() const
	{
		if(info.defaultValue) {
			return *info.defaultValue;
		}
		return nullptr;
	}

	String getStringValue() const;

	Type getType() const
	{
		return info.type;
	}

	explicit operator bool() const
	{
		return object != nullptr;
	}

	String getJsonValue() const;

private:
	Object* object{};
	PropertyInfo info{};
	uint16_t index{};
};

} // namespace ConfigDB

String toString(ConfigDB::PropertyType type);
