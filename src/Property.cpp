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

String toString(ConfigDB::PropertyType type)
{
	switch(type) {
#define XX(name, ...)                                                                                                  \
	case ConfigDB::PropertyType::name:                                                                                 \
		return F(#name);
		CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
	}
	return nullptr;
}

namespace ConfigDB
{
const PropertyInfo PropertyInfo::empty PROGMEM{};

String Property::getValue() const
{
	if(!object) {
		return nullptr;
	}

	String value; // = isIndexed() ? object->getStoredArrayValue(index) : object->getStoredValue(info.getName());
	return value ?: info.getDefaultValue();
}

String Property::getJsonValue() const
{
	if(!object) {
		return nullptr;
	}
	String value = getValue();
	if(!value) {
		return "null";
	}
	if(info.type < Type::String) {
		return value;
	}
	::Format::standard.quote(value);
	return value;
}

} // namespace ConfigDB
