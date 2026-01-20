/****
 * ConfigDB/PropertyData.h
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
	Number number;
	ArrayId array;
	StringId string;

	String getString(const PropertyInfo& info) const;

	bool setValue(const PropertyInfo& prop, int64_t value);

	bool setValue(const PropertyInfo& prop, Number value);

	bool setValue(const PropertyInfo& prop, const char* value, unsigned valueLength);

	static PropertyData* fromStruct(const PropertyInfo& prop, void* data)
	{
		return data ? reinterpret_cast<PropertyData*>(static_cast<uint8_t*>(data) + prop.offset) : nullptr;
	}

	static const PropertyData* fromStruct(const PropertyInfo& prop, const void* data)
	{
		return fromStruct(prop, const_cast<void*>(data));
	}
};

} // namespace ConfigDB
