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
 * @brief Manages a key/value pair stored in an object, or a simple array value
 */
class PropertyConst
{
public:
	PropertyConst() = default;

	/**
	 * @brief Create a Property instance
	 * @param info Property information
	 * @param data Pointer to location where value is stored
	 */
	PropertyConst(const Store& store, const PropertyInfo& info, const PropertyData* data,
				  const PropertyData* defaultData)
		: propinfo(&info), store(&store), data(data), defaultData(defaultData)
	{
	}

	String getValue() const;

	explicit operator bool() const
	{
		return store;
	}

	String getJsonValue() const;

	const PropertyInfo& info() const
	{
		return *propinfo;
	}

protected:
	const PropertyInfo* propinfo{&PropertyInfo::empty};
	const Store* store{};
	const PropertyData* data{};
	const PropertyData* defaultData{};
};

class Property : public PropertyConst
{
public:
	using PropertyConst::PropertyConst;

	bool setJsonValue(const char* value, size_t valueLength);

	bool setJsonValue(const String& value)
	{
		return setJsonValue(value.c_str(), value.length());
	}
};

} // namespace ConfigDB
