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
	auto dst = const_cast<void*>(data);
	if(value) {
		auto propdata = const_cast<Store*>(store)->parseString(*info, value, valueLength);
		memcpy(dst, &propdata, info->getSize());
	} else if(defaultData) {
		memcpy_P(dst, defaultData, info->getSize());
	} else {
		memset(dst, 0, info->getSize());
	}
	return true;
}

void PropertyData::setValue(const PropertyInfo& prop, const PropertyData& src)
{
	auto clipInt32 = [&prop](int32_t value) { return TRange(prop.minimum.Int32, prop.maximum.Int32).clip(value); };
	auto clipUInt32 = [&prop](uint32_t value) { return TRange(prop.minimum.UInt32, prop.maximum.UInt32).clip(value); };

	switch(prop.type) {
	case PropertyType::Boolean:
		b = src.b;
		break;
	case PropertyType::Int8:
		int8 = clipInt32(src.int8);
		break;
	case PropertyType::Int16:
		int16 = clipInt32(src.int16);
		break;
	case PropertyType::Int32:
		int32 = clipInt32(src.int32);
		break;
	case PropertyType::Int64:
		if(prop.minimum.Int64 || prop.maximum.Int64) {
			int64 = TRange(getPtrValue(prop.minimum.Int64), getPtrValue(prop.maximum.Int64)).clip(src.int64);
		} else {
			int64 = src.int64;
		}
		break;
	case PropertyType::UInt8:
		uint8 = clipUInt32(src.uint8);
		break;
	case PropertyType::UInt16:
		uint16 = clipUInt32(src.uint16);
		break;
	case PropertyType::UInt32:
		uint32 = clipUInt32(src.uint32);
		break;
	case PropertyType::UInt64:
		if(prop.minimum.UInt64 || prop.maximum.UInt64) {
			uint64 = TRange(getPtrValue(prop.minimum.UInt64), getPtrValue(prop.maximum.UInt64)).clip(src.uint64);
		} else {
			uint64 = src.uint64;
		}
		break;
	case PropertyType::String:
		assert(false);
		string = src.string;
		break;
	}
}

} // namespace ConfigDB
