/**
 * ConfigDB/Property.cpp
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

#include "include/ConfigDB/Database.h"
#include <debug_progmem.h>

namespace ConfigDB
{
void Store::clear()
{
	auto& root = typeinfo();
	memcpy_P(rootData.get(), root.defaultData, root.structSize);
	stringPool.clear();
	arrayPool.clear();
}

String Store::getFilePath() const
{
	String path = db.getPath();
	path += '/';
	path += getFileName();
	return path;
}

String Store::getValueString(const PropertyInfo& info, const void* data) const
{
	auto& propData = *static_cast<const PropertyData*>(data);
	switch(info.type) {
	case PropertyType::Boolean:
		return propData.b ? "true" : "false";
	case PropertyType::Int8:
		return String(propData.int8);
	case PropertyType::Int16:
		return String(propData.int16);
	case PropertyType::Int32:
		return String(propData.int32);
	case PropertyType::Int64:
		return String(propData.int64);
	case PropertyType::UInt8:
		return String(propData.uint8);
	case PropertyType::UInt16:
		return String(propData.uint16);
	case PropertyType::UInt32:
		return String(propData.uint32);
	case PropertyType::UInt64:
		return String(propData.uint64);
	case PropertyType::String:
		if(propData.string) {
			return String(stringPool[propData.string]);
		}
		if(info.defaultValue) {
			return *info.defaultValue;
		}
		return nullptr;
	}
	return nullptr;
}

PropertyData Store::parseString(const PropertyInfo& prop, const char* value, uint16_t valueLength)
{
	switch(prop.type) {
	case PropertyType::Boolean:
		return {.b = (valueLength == 4) && memicmp(value, "true", 4) == 0};
	case PropertyType::Int8:
	case PropertyType::Int16:
	case PropertyType::Int32:
		return {.int32 = strtol(value, nullptr, 0)};
	case PropertyType::Int64:
		return {.int64 = strtoll(value, nullptr, 0)};
	case PropertyType::UInt8:
	case PropertyType::UInt16:
	case PropertyType::UInt32:
		return {.uint32 = strtoul(value, nullptr, 0)};
	case PropertyType::UInt64:
		return {.uint64 = strtoull(value, nullptr, 0)};
	case PropertyType::String:
		if(prop.defaultValue && *prop.defaultValue == value) {
			return {.string = 0};
		}
		return {.string = stringPool.findOrAdd({value, valueLength})};
	}

	return {};
}

bool Store::writeCheck() const
{
	if(readOnly) {
		debug_e("[CFGDB] Store is Read-only");
		return false;
	}
	return true;
}

} // namespace ConfigDB
