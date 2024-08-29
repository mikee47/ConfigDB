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

/**
 * @brief Identifies array storage within array pool
 * @note Can't just use uint16_t as it may be unaligned.
 * Using `alignas` doesn't help.
 */
struct __attribute__((packed)) ArrayId {
	uint8_t value_[2];

	constexpr ArrayId(uint16_t value = 0) : value_{uint8_t(value), uint8_t(value >> 8)}
	{
	}

	constexpr operator uint16_t() const
	{
		return value_[0] | (value_[1] << 8);
	}
};

union __attribute__((packed)) PropertyData {
	uint8_t uint8;
	uint16_t uint16;
	uint32_t uint32;
	uint64_t uint64;
	int8_t int8;
	int16_t int16;
	int32_t int32;
	int64_t int64;
	bool boolean;
	float f;
	ArrayId array;
	StringId string;

	/**
	 * @brief Range-check raw binary value. Do not use with Strings.
	 */
	void setValue(const PropertyInfo& prop, const PropertyData& src);

	static PropertyData* fromStruct(const PropertyInfo& prop, void* data)
	{
		return data ? reinterpret_cast<PropertyData*>(static_cast<uint8_t*>(data) + prop.offset) : nullptr;
	}

	static const PropertyData* fromStruct(const PropertyInfo& prop, const void* data)
	{
		return fromStruct(prop, const_cast<void*>(data));
	}
};

/**
 * @brief Manages a key/value pair stored in an object, or a simple array value
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
	PropertyConst(const Store& store, const PropertyInfo& info, const PropertyData* data,
				  const PropertyData* defaultData)
		: info(&info), store(&store), data(data), defaultData(defaultData)
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
