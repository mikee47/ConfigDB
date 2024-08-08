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
	XX(Boolean)                                                                                                        \
	XX(Int8)                                                                                                           \
	XX(Int16)                                                                                                          \
	XX(Int32)                                                                                                          \
	XX(Int64)                                                                                                          \
	XX(UInt8)                                                                                                          \
	XX(UInt16)                                                                                                         \
	XX(UInt32)                                                                                                         \
	XX(UInt64)                                                                                                         \
	XX(String)

namespace ConfigDB
{
class Object;

enum class PropertyType {
#define XX(name) name,
	CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
};

/**
 * @brief Property metadata
 */
struct PropertyInfo {
	// Don't access these directly!
	const FlashString* name;
	const FlashString* defaultValue;
	PropertyType type;

	String getName() const
	{
		return name ? String(*name) : nullptr;
	}

	bool operator==(const String& s) const
	{
		return name && *name == s;
	}

	String getDefaultValue() const
	{
		return defaultValue ? String(*defaultValue) : nullptr;
	}

	PropertyType getType() const
	{
		return FSTR::readValue(&type);
	}

	explicit operator bool() const
	{
		return name != nullptr;
	}
};

static constexpr const PropertyInfo emptyPropertyInfo{};

/**
 * @brief Manages a key/value pair stored in an object
 */
class Property
{
public:
	Property() : info(emptyPropertyInfo)
	{
	}

	using Type = PropertyType;

	/**
	 * @brief Property accessed by key
	 */
	Property(Object& object, const PropertyInfo& info) : info(info), object(&object)
	{
	}

	/**
	 * @brief Property accessed by index
	 */
	Property(Object& object, unsigned index, const PropertyInfo& info) : info(info), object(&object), index(index)
	{
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
		return index >= 0;
	}

	String getValue() const;

	explicit operator bool() const
	{
		return object != nullptr;
	}

	String getJsonValue() const;

	const PropertyInfo& info;

private:
	Object* object{};
	int index{-1};
};

} // namespace ConfigDB

String toString(ConfigDB::PropertyType type);
