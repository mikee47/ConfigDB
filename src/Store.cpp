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
	path += name ?: F("_root");
	return path;
}

void* Store::getObjectDataPtr(const ObjectInfo& object)
{
	return rootObjectData.get() + object.getOffset();
}

void* Store::getObjectArrayDataPtr(const ObjectInfo& object, ArrayId arrayId, unsigned index)
{
	if(arrayId == 0) {
		return nullptr;
	}
	auto& array = objectArrayPool[arrayId];
	if(index == array.getCount()) {
		auto& items = object.objinfo->valueAt(0);
		array.add(items.getStructSize(), items.defaultData);
	}
	return array[index].get();
}

String Store::getValueString(const PropertyInfo& info, const void* data) const
{
	auto& propData = *reinterpret_cast<const PropertyData*>(data);
	switch(info.getType()) {
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
		return propData.string ? stringPool[propData.string] : info.getDefaultValue();
	}
	return nullptr;
}

} // namespace ConfigDB
