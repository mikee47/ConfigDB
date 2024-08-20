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

#include "PropertyInfo.h"

namespace ConfigDB
{
class Store;

union __attribute__((packed)) PropertyData {
	uint8_t uint8;
	uint16_t uint16;
	uint32_t uint32;
	uint64_t uint64;
	int8_t int8;
	int16_t int16;
	int32_t int32;
	int64_t int64;
	bool b;
	float f;
	StringId string;
};

/**
 * @brief Manages a key/value pair stored in an object
 */
class PropertyConst
{
public:
	PropertyConst() : info(&PropertyInfo::empty)
	{
	}

	/**
	 * @brief Create a Property instance
	 * @param info Property information
	 * @param data Pointer to location where value is stored
	 */
	PropertyConst(Store& store, const PropertyInfo& info, void* data) : info(&info), store(&store), data(data)
	{
	}

	String getValue() const;

	explicit operator bool() const
	{
		return store;
	}

	String getJsonValue() const;

	const PropertyInfo& typeinfo() const
	{
		return *info;
	}

protected:
	const PropertyInfo* info;
	Store* store{};
	void* data{};
};

class Property : public PropertyConst
{
public:
	using PropertyConst::PropertyConst;

	bool setJsonValue(const char* value, size_t valueLength);
};

} // namespace ConfigDB
