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

namespace ConfigDB
{
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
	case PropertyType::String:
		assert(false);
		string = src.string;
		break;
	case PropertyType::Object:
		assert(false);
		break;
	}
}

} // namespace ConfigDB
