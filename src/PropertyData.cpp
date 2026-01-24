/****
 * ConfigDB/PropertyData.cpp
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

#include "include/ConfigDB/PropertyData.h"

namespace ConfigDB
{
/**
 * @brief Clamp an integer to the range of a specific storage type
 * @tparam T Type of integer value is to be clamped to
 */
template <typename T> constexpr T clamp(int64_t value)
{
	using L = std::numeric_limits<T>;
	return TRange<int64_t>{L::min(), L::max()}.clip(value);
}

String PropertyData::getString(const PropertyInfo& info) const
{
	switch(info.type) {
	case PropertyType::Boolean:
		return boolean ? "true" : "false";
	case PropertyType::Enum:
		return info.variant.enuminfo->getString(uint8);
	case PropertyType::Int8:
		return String(int8);
	case PropertyType::Int16:
		return String(int16);
	case PropertyType::Int32:
		return String(int32);
	case PropertyType::Int64:
		return String(int64);
	case PropertyType::UInt8:
		return String(uint8);
	case PropertyType::UInt16:
		return String(uint16);
	case PropertyType::UInt32:
		return String(uint32);
	case PropertyType::Number:
		return String(number);
	case PropertyType::String:
	case PropertyType::Object:
	case PropertyType::Alias:
		break;
	}
	assert(false);
	return nullptr;
}

bool PropertyData::setValue(const PropertyInfo& prop, int64_t value)
{
	switch(prop.type) {
	case PropertyType::Boolean:
		boolean = (value != 0);
		return true;
	case PropertyType::Enum:
		uint8 = TRange<int64_t>(0, prop.variant.enuminfo->length() - 1).clip(value);
		return true;
	case PropertyType::Int8:
		int8 = prop.variant.int8 ? prop.variant.int8->clip(value) : clamp<int8_t>(value);
		return true;
	case PropertyType::Int16:
		int16 = prop.variant.int16 ? prop.variant.int16->clip(value) : clamp<int16_t>(value);
		return true;
	case PropertyType::Int32:
		int32 = prop.variant.int32 ? prop.variant.int32->clip(value) : clamp<int32_t>(value);
		return true;
	case PropertyType::Int64:
		int64 = prop.variant.int64 ? prop.variant.int64->clip(value) : value;
		return true;
	case PropertyType::UInt8:
		uint8 = prop.variant.uint8 ? prop.variant.uint8->clip(value) : clamp<uint8_t>(value);
		return true;
	case PropertyType::UInt16:
		uint16 = prop.variant.uint16 ? prop.variant.uint16->clip(value) : clamp<uint16_t>(value);
		return true;
	case PropertyType::UInt32:
		uint32 = prop.variant.uint32 ? prop.variant.uint32->clip(value) : clamp<uint32_t>(value);
		return true;
	case PropertyType::Number: {
		number_t num = Number{value};
		number = prop.variant.number ? prop.variant.number->clip(num)
									 : TRange<number_t>(number_t::lowest(), number_t::max()).clip(num);
		return true;
	}
	case PropertyType::String:
		string = value;
		return true;
	case PropertyType::Object:
	case PropertyType::Alias:
		break;
	}

	assert(false);
	return false;
}

bool PropertyData::setValue(const PropertyInfo& prop, Number value)
{
	if(prop.type == PropertyType::Number) {
		number = prop.variant.number ? prop.variant.number->clip(number_t(value)) : value;
		return true;
	}

	assert(false);
	return false;
}

bool PropertyData::setValue(const PropertyInfo& prop, const char* value, unsigned valueLength)
{
	switch(prop.type) {
	case PropertyType::Boolean:
		boolean = (valueLength == 4) && memicmp(value, "true", 4) == 0;
		return true;
	case PropertyType::Int8:
	case PropertyType::Int16:
	case PropertyType::Int32:
	case PropertyType::Int64:
	case PropertyType::UInt8:
	case PropertyType::UInt16:
	case PropertyType::UInt32:
		return setValue(prop, strtoll(value, nullptr, 0));
	case PropertyType::Number: {
		number_t num{};
		if(number_t::parse(value, valueLength, num)) {
			return setValue(prop, value);
		}
		return false;
	}
	case PropertyType::Enum: {
		if(!value) {
			uint8 = 0;
			return true;
		}
		int i = prop.variant.enuminfo->find(value, valueLength);
		if(i < 0) {
			return false;
		}
		uint8 = uint8_t(i);
		return true;
	}
	case PropertyType::String:
	case PropertyType::Object:
	case PropertyType::Alias:
		break;
	}

	assert(false);
	return false;
}

} // namespace ConfigDB
