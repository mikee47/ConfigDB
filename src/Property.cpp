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
const PropertyInfo PropertyInfo::empty PROGMEM{.name = fstr_empty};

String toString(PropertyType type)
{
	switch(type) {
#define XX(name, ...)                                                                                                  \
	case PropertyType::name:                                                                                           \
		return F(#name);
		CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
	}
	return nullptr;
}

String PropertyConst::getValue() const
{
	assert(info && store && data);
	if(!store) {
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

bool Property::setValueString(const char* value, size_t valueLength)
{
	assert(info && store && data);
	if(!store) {
		return false;
	}
	store->setValueString(*info, data, value, valueLength);
	return true;
}

} // namespace ConfigDB
