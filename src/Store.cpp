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

namespace ConfigDB
{
String Store::getPath() const
{
	String path = db.getPath();
	path += '/';
	path += isRoot() ? F("_root") : getName();
	return path;
}

void* Store::getObjectDataPtr(const ObjectInfo& object)
{
	return rootObjectData.get() + object.getOffset();
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
			return stringPool[propData.string];
		}
		if(info.defaultValue) {
			return *info.defaultValue;
		}
		return nullptr;
	}
	return nullptr;
}

bool Store::setValueString(const PropertyInfo& prop, void* data, const String& value)
{
	ConfigDB::PropertyData propdata{};
	switch(prop.type) {
	case PropertyType::Boolean:
		propdata.b = value.equalsIgnoreCase("true");
		break;
	case PropertyType::Int8:
	case PropertyType::Int16:
	case PropertyType::Int32:
		propdata.int32 = strtol(value.c_str(), nullptr, 0);
		break;
	case PropertyType::Int64:
		propdata.int32 = strtoll(value.c_str(), nullptr, 0);
		break;
	case PropertyType::UInt8:
	case PropertyType::UInt16:
	case PropertyType::UInt32:
		propdata.uint32 = strtoul(value.c_str(), nullptr, 0);
		break;
	case PropertyType::UInt64:
		propdata.uint64 = strtoull(value.c_str(), nullptr, 0);
		break;
	case PropertyType::String: {
		if(prop.defaultValue && *prop.defaultValue == value) {
			propdata.string = 0;
		} else {
			auto ref = stringPool.findOrAdd(value);
			propdata.string = ref;
		}
		break;
	}
	}

	memcpy(data, &propdata, prop.getSize());
	return true;
}

} // namespace ConfigDB
