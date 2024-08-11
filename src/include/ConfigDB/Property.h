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

/**
 * @brief Property types with storage size
 */
#define CONFIGDB_PROPERTY_TYPE_MAP(XX)                                                                                 \
	XX(Boolean, 1)                                                                                                     \
	XX(Int8, 1)                                                                                                        \
	XX(Int16, 2)                                                                                                       \
	XX(Int32, 4)                                                                                                       \
	XX(Int64, 8)                                                                                                       \
	XX(UInt8, 1)                                                                                                       \
	XX(UInt16, 2)                                                                                                      \
	XX(UInt32, 4)                                                                                                      \
	XX(UInt64, 8)                                                                                                      \
	XX(String, sizeof(StringId))

namespace ConfigDB
{
class Object;

/**
 * @brief Defines contained string data using index into string pool
 */
using StringId = uint16_t;

enum class PropertyType : uint32_t {
#define XX(name, ...) name,
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
	volatile PropertyType type : 8;

	static const PropertyInfo empty;

	String getName() const
	{
		return name ? String(*name) : nullptr;
	}

	bool nameIs(const char* value, size_t length) const
	{
		return name ? name->equals(value, length) : (length == 0);
	}

	bool operator==(const String& s) const
	{
		return name && *name == s;
	}

	String getDefaultValue() const
	{
		return defaultValue ? String(*defaultValue) : nullptr;
	}

	/**
	 * @brief Get number of bytes required to store this property value within a structure
	 */
	uint8_t getSize() const
	{
		switch(type) {
#define XX(tag, size)                                                                                                  \
	case PropertyType::tag:                                                                                            \
		return size;
			CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
		}
		return 0;
	}
};

union __attribute__((packed)) PropertyData {
	uint8_t uint8;
	uint16_t uint16;
	uint32_t uint32;
	uint16_t uint64;
	int8_t int8;
	int16_t int16;
	int32_t int32;
	int16_t int64;
	bool b;
	float f;
	StringId string;
};

/**
 * @brief Manages a key/value pair stored in an object
 */
class Property
{
public:
	Property() : info(PropertyInfo::empty)
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
