/****
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

#include "PropertyData.h"

namespace ConfigDB
{
class Store;

/**
 * @brief Read-only access to a key/value pair stored in an object, or a simple array value
 */
class PropertyConst
{
public:
	PropertyConst() = default;

	/**
	 * @brief Create a PropertyConst instance
	 * @param info Property information
	 * @param data Pointer to location where value is stored
	 */
	PropertyConst(const Store& store, const PropertyInfo& info, const PropertyData* data)
		: propinfo(&info), store(&store), data(data)
	{
	}

	/**
	 * @brief Retrieve the property value as a string
	 */
	String getValue() const;

	explicit operator bool() const
	{
		return store;
	}

	/**
	 * @brief Get property value as a valid JSON string
	 * @retval String Can return "null", numeric, boolean or *quoted* string
	 */
	String getJsonValue() const;

	/**
	 * @brief Access property type information
	 */
	const PropertyInfo& info() const
	{
		return *propinfo;
	}

protected:
	const PropertyInfo* propinfo{&PropertyInfo::empty};
	const Store* store{};
	const PropertyData* data{};
};

/**
 * @brief Adds write access for a key/value pair stored in an object, or a simple array value
 */
class Property : public PropertyConst
{
public:
	Property() = default;

	/**
	 * @brief Create a Property instance
	 * @param info Property information
	 * @param data Pointer to location where value is stored
	 * @param defaultData Pointer to default value for property (if any)
	 */
	Property(const Store& store, const PropertyInfo& info, const PropertyData* data, const PropertyData* defaultData)
		: PropertyConst(store, info, data), defaultData(defaultData)
	{
	}

	/**
	 * @brief Set property value given its JSON string representation
	 * @param value String values must be *unquoted*
	 * @param valueLength Length of value in characters
	 * @retval bool Return false if this property instance or the given value are invalid
	 */
	bool setJsonValue(const char* value, size_t valueLength);

	bool setJsonValue(const String& value)
	{
		return setJsonValue(value.c_str(), value.length());
	}

protected:
	const PropertyData* defaultData{};
};

} // namespace ConfigDB
