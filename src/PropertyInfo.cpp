/****
 * ConfigDB/PropertyInfo.cpp
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

#include "include/ConfigDB/PropertyInfo.h"
#include "include/ConfigDB/ObjectInfo.h"
#include "include/ConfigDB/PropertyData.h"

String toString(ConfigDB::PropertyType type)
{
	switch(type) {
#define XX(name, ...)                                                                                                  \
	case ConfigDB::PropertyType::name:                                                                                 \
		return F(#name);
		CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
	}
	return nullptr;
}

namespace ConfigDB
{
const PropertyInfo PropertyInfo::empty PROGMEM{.name = fstr_empty};

String EnumInfo::getString(uint8_t index) const
{
	switch(type) {
	case PropertyType::Int8:
		return String(getValue<int8_t>(index));
	case PropertyType::Int16:
		return String(getValue<int16_t>(index));
	case PropertyType::Int32:
		return String(getValue<int32_t>(index));
	case PropertyType::Int64:
		return String(getValue<int64_t>(index));
	case PropertyType::UInt8:
		return String(getValue<uint8_t>(index));
	case PropertyType::UInt16:
		return String(getValue<uint16_t>(index));
	case PropertyType::UInt32:
		return String(getValue<uint32_t>(index));
	case PropertyType::UInt64:
		return String(getValue<uint64_t>(index));
	case PropertyType::Number:
		return toString(getValue<number_t>(index));
	case PropertyType::String:
		return getStrings()[index];
	case PropertyType::Boolean:
	case PropertyType::Enum:
	case PropertyType::Object:
	case PropertyType::Alias:
		break;
	}
	assert(false);
	return nullptr;
}

int EnumInfo::find(const char* value, unsigned length) const
{
	if(type == PropertyType::String) {
		return getStrings().indexOf(value, length);
	}

	if(type == PropertyType::Number) {
		number_t num{};
		if(!number_t::parse(value, length, num)) {
			return -1;
		}
		return getArray<number_t>().indexOf(num);
	}

	if(type == PropertyType::UInt64) {
		uint64_t intval = strtoull(value, nullptr, 0);
		return getArray<uint64_t>().indexOf(intval);
	}

	int64_t intval = strtoll(value, nullptr, 0);
	switch(type) {
	case PropertyType::Int8:
		return getArray<int8_t>().indexOf(intval);
	case PropertyType::Int16:
		return getArray<int16_t>().indexOf(intval);
	case PropertyType::Int32:
		return getArray<int32_t>().indexOf(intval);
	case PropertyType::Int64:
		return getArray<int64_t>().indexOf(intval);
	case PropertyType::UInt8:
		return getArray<uint8_t>().indexOf(intval);
	case PropertyType::UInt16:
		return getArray<uint16_t>().indexOf(intval);
	case PropertyType::UInt32:
		return getArray<uint32_t>().indexOf(intval);
	case PropertyType::UInt64:
	case PropertyType::Number:
	case PropertyType::String:
	case PropertyType::Boolean:
	case PropertyType::Enum:
	case PropertyType::Object:
	case PropertyType::Alias:
		break;
	}
	assert(false);
	return -1;
}

int PropertyInfo::findObject(const char* name, unsigned length) const
{
	if(type != PropertyType::Object) {
		return -1;
	}
	return variant.object->findObject(name, length);
}

int PropertyInfo::findProperty(const char* name, unsigned length) const
{
	if(type != PropertyType::Object) {
		return -1;
	}
	return variant.object->findProperty(name, length);
}

const PropertyInfo& PropertyInfo::getObject(unsigned index) const
{
	assert(type == PropertyType::Object);
	return variant.object->getObject(index);
}

} // namespace ConfigDB
