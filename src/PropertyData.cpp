/**
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
#include <stringconversion.h>

namespace ConfigDB
{
String PropertyData::getString(const PropertyInfo& info) const
{
	switch(info.type) {
	case PropertyType::Boolean:
		return boolean ? "true" : "false";
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
		break;
	}
	assert(false);
	return nullptr;
}

void PropertyData::setValue(const PropertyInfo& prop, const PropertyData& src)
{
	switch(prop.type) {
	case PropertyType::Boolean:
		boolean = src.boolean;
		break;
	case PropertyType::Int8:
		int8 = prop.int32.clip(src.int8);
		break;
	case PropertyType::Int16:
		int16 = prop.int32.clip(src.int16);
		break;
	case PropertyType::Int32:
		int32 = prop.int32.clip(src.int32);
		break;
	case PropertyType::Int64:
		int64 = prop.int64.clip(src.int64);
		break;
	case PropertyType::UInt8:
		uint8 = prop.uint32.clip(src.uint8);
		break;
	case PropertyType::UInt16:
		uint16 = prop.uint32.clip(src.uint16);
		break;
	case PropertyType::UInt32:
		uint32 = prop.uint32.clip(src.uint32);
		break;
	case PropertyType::UInt64:
		uint64 = prop.uint64.clip(src.uint64);
		break;
	case PropertyType::Number:
		number = prop.number.clip(src.number);
		break;
	case PropertyType::String:
		string = src.string;
		break;
	case PropertyType::Object:
		assert(false);
		break;
	}
}

bool PropertyData::setValue(PropertyType type, const char* value, unsigned valueLength)
{
	switch(type) {
	case PropertyType::Boolean:
		boolean = (valueLength == 4) && memicmp(value, "true", 4) == 0;
		return true;
	case PropertyType::Int8:
		int8 = strtol(value, nullptr, 0);
		return true;
	case PropertyType::Int16:
		int16 = strtol(value, nullptr, 0);
		return true;
	case PropertyType::Int32:
		int32 = strtol(value, nullptr, 0);
		return true;
	case PropertyType::Int64:
		int64 = strtoll(value, nullptr, 0);
		return true;
	case PropertyType::UInt8:
		uint8 = strtoul(value, nullptr, 0);
		return true;
	case PropertyType::UInt16:
		uint16 = strtoul(value, nullptr, 0);
		return true;
	case PropertyType::UInt32:
		uint32 = strtoul(value, nullptr, 0);
		return true;
	case PropertyType::UInt64:
		uint64 = strtoull(value, nullptr, 0);
		return true;
	case PropertyType::Number: {
		number_t num{};
		if(Number::parse(value, valueLength, num)) {
			number = num;
			return true;
		}
		return false;
	}
	case PropertyType::String:
	case PropertyType::Object:
		break;
	}
	assert(false);
	return false;
}

bool PropertyData::setValue(const PropertyInfo& prop, const char* value, unsigned valueLength)
{
	PropertyData src{};
	if(!src.setValue(prop.type, value, valueLength)) {
		return false;
	}
	setValue(prop, src);
	return true;
}

} // namespace ConfigDB
