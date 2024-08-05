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
#include "include/ConfigDB/Object.h"
#include <Data/Format/Standard.h>

String toString(ConfigDB::Property::Type type)
{
	switch(type) {
#define XX(name)                                                                                                       \
	case ConfigDB::Property::Type::name:                                                                               \
		return F(#name);
		CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
	}
	return nullptr;
}

namespace ConfigDB
{
String Property::getStringValue() const
{
	if(!object) {
		return nullptr;
	}

	String value;
	if(info.name) {
		value = object->getStringValue(*info.name);
	} else {
		value = object->getStringValue(index);
	}
	if(!value && info.defaultValue) {
		value = *info.defaultValue;
	}
	return value;
}

std::unique_ptr<Object> Property::getObjectValue() const
{
	if(!object) {
		return nullptr;
	}

	if(!info.name) {
		return object->getObject(index);
	}

	assert(false);

	auto& typeinfo = object->getTypeinfo();
	if(!typeinfo.objinfo) {
		return nullptr;
	}
	unsigned i = 0;
	for(auto& obj : *typeinfo.objinfo) {
		if(obj.name == info.name) {
			return object->getObject(i);
		}
		++i;
	}
	return nullptr;
}

String Property::getJsonValue() const
{
	if(!object) {
		return nullptr;
	}
	String value = getStringValue();
	if(!value) {
		return "null";
	}
	switch(info.type) {
	case Type::Integer:
	case Type::Boolean:
		return value;
	case Type::String:
		break;
	case Type::Object:
	case Type::Array:
	case Type::ObjectArray:
		return value;
	}
	::Format::standard.quote(value);
	return value;
}

} // namespace ConfigDB
