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
	assert(info && store && data);
	if(!store) {
		return false;
	}
	auto propdata = const_cast<Store*>(store)->parseString(*info, value, valueLength);
	memcpy(const_cast<void*>(data), &propdata, info->getSize());
	return true;
}

} // namespace ConfigDB
