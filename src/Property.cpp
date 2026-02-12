/****
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
String Property::getValue() const
{
	assert(propinfo && store);
	if(!store || !data) {
		return nullptr;
	}
	return store->getValueString(*propinfo, data);
}

String Property::getJsonValue() const
{
	String value = getValue();
	if(!value) {
		return "null";
	}
	if(propinfo->isStringType()) {
		::Format::json.quote(value);
	}
	return value;
}

bool Property::setJsonValue(const char* value, size_t valueLength)
{
	assert(propinfo && store);
	if(!store || !data) {
		return false;
	}
	auto& dst = *const_cast<PropertyData*>(data);
	return const_cast<Store*>(store)->parseString(*propinfo, dst, defaultData, value, valueLength);
}

} // namespace ConfigDB
