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
	case PropertyType::UInt64:
		return String(uint64);
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

void PropertyData::setValue(const PropertyInfo& prop, const int64_t& value)
{
	switch(prop.type) {
	case PropertyType::Boolean:
		boolean = (value != 0);
		break;
	case PropertyType::Enum:
		uint8 = TRange<int64_t>(0, prop.variant.enuminfo->length() - 1).clip(value);
		break;
	case PropertyType::Int8:
		int8 = PropertyInfo::RangeInt8::clip(prop.variant.int8, value);
		break;
	case PropertyType::Int16:
		int16 = PropertyInfo::RangeInt16::clip(prop.variant.int16, value);
		break;
	case PropertyType::Int32:
		int32 = PropertyInfo::RangeInt32::clip(prop.variant.int32, value);
		break;
	case PropertyType::Int64:
		int64 = PropertyInfo::RangeInt64::clip(prop.variant.int64, value);
		break;
	case PropertyType::UInt8:
		uint8 = PropertyInfo::RangeUInt8::clip(prop.variant.uint8, value);
		break;
	case PropertyType::UInt16:
		uint16 = PropertyInfo::RangeUInt16::clip(prop.variant.uint16, value);
		break;
	case PropertyType::UInt32:
		uint32 = PropertyInfo::RangeUInt32::clip(prop.variant.uint32, value);
		break;
	case PropertyType::UInt64:
		uint64 = PropertyInfo::RangeUInt64::clip(prop.variant.uint64, value);
		break;
	case PropertyType::Number:
		number = PropertyInfo::RangeNumber::clip(prop.variant.number, Number{value});
		break;
	case PropertyType::String:
		string = value;
		break;
	case PropertyType::Object:
	case PropertyType::Alias:
		assert(false);
		break;
	}
}

void PropertyData::setValue(const PropertyInfo& prop, const Number& value)
{
	if(prop.type == PropertyType::Number) {
		number = PropertyInfo::RangeNumber::clip(prop.variant.number, value);
		return;
	}

	assert(false);
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
	case PropertyType::UInt64:
		setValue(prop, strtoll(value, nullptr, 0));
		return true;
	case PropertyType::Number: {
		number_t num{};
		if(number_t::parse(value, valueLength, num)) {
			number = PropertyInfo::RangeNumber::clip(prop.variant.number, num);
			return true;
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
