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

#include "include/ConfigDB/Property.h"
#include "include/ConfigDB/Store.h"
#include <Data/Format/Json.h>
#include <Data/Range.h>

namespace
{
template <typename T> T getPtrValue(const T* value)
{
	return value ? *value : 0;
}

} // namespace

namespace ConfigDB
{
String PropertyConst::getValue() const
{
	assert(info && store);
	if(!store || !data) {
		return nullptr;
	}
	return store->getValueString(*info, data);
}

String PropertyConst::getJsonValue() const
{
	String value = getValue();
	if(!value) {
		return "null";
	}
	if(info->type < PropertyType::String) {
		return value;
	}
	::Format::json.escape(value);
	::Format::json.quote(value);
	return value;
}

bool Property::setJsonValue(const char* value, size_t valueLength)
{
	assert(info && store);
	if(!store || !data) {
		return false;
	}
	auto propdata = const_cast<Store*>(store)->parseString(*info, value, valueLength);
	memcpy(const_cast<void*>(data), &propdata, info->getSize());
	return true;
}

void PropertyData::setValue(const PropertyInfo& prop, const PropertyData* src)
{
	switch(prop.type) {
	case PropertyType::Boolean:
		b = src ? src->b : (prop.defaultValue.Int32 != 0);
		break;
	case PropertyType::Int8:
		int8 = src ? TRange(prop.minimum.Int32, prop.maximum.Int32).clip(src->int8) : prop.defaultValue.Int32;
		break;
	case PropertyType::Int16:
		int16 = src ? TRange(prop.minimum.Int32, prop.maximum.Int32).clip(src->int16) : prop.defaultValue.Int32;
		break;
	case PropertyType::Int32:
		int32 = src ? TRange(prop.minimum.Int32, prop.maximum.Int32).clip(src->int32) : prop.defaultValue.Int32;
		break;
	case PropertyType::Int64:
		int64 = src ? src->int64 : getPtrValue(prop.defaultValue.Int64);
	case PropertyType::UInt8:
		uint8 = src ? TRange(prop.minimum.UInt32, prop.maximum.UInt32).clip(src->uint8) : prop.defaultValue.UInt32;
		break;
	case PropertyType::UInt16:
		uint16 = src ? TRange(prop.minimum.UInt32, prop.maximum.UInt32).clip(src->uint16) : prop.defaultValue.UInt32;
		break;
	case PropertyType::UInt32:
		uint32 = src ? TRange(prop.minimum.UInt32, prop.maximum.UInt32).clip(src->uint32) : prop.defaultValue.UInt32;
		break;
	case PropertyType::UInt64:
		uint64 = src ? src->uint64 : getPtrValue(prop.defaultValue.UInt64);
	case PropertyType::String:
		assert(false);
		break;
	}
}

} // namespace ConfigDB
