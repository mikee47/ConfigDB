/****
 * ConfigDB/ObjectInfo.cpp
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

#include "include/ConfigDB/ObjectInfo.h"

String toString(ConfigDB::ObjectType type)
{
	switch(type) {
#define XX(name)                                                                                                       \
	case ConfigDB::ObjectType::name:                                                                                   \
		return F(#name);
		CONFIGDB_OBJECT_TYPE_MAP(XX)
#undef XX
	}
	return nullptr;
}

namespace ConfigDB
{
const ObjectInfo PROGMEM ObjectInfo::empty{};

int ObjectInfo::findObject(const char* name, size_t length) const
{
	for(unsigned i = 0; i < objectCount; ++i) {
		if(propinfo[i].name.equals(name, length)) {
			return i;
		}
	}
	int i = findAlias(name, length);
	return (i < int(objectCount)) ? i : -1;
}

int ObjectInfo::findProperty(const char* name, size_t length) const
{
	for(unsigned i = 0; i < propertyCount; ++i) {
		if(propinfo[objectCount + i].name.equals(name, length)) {
			return i;
		}
	}
	return findAlias(name, length) - objectCount;
}

int ObjectInfo::findAlias(const char* name, size_t length) const
{
	for(unsigned i = 0; i < aliasCount; ++i) {
		auto& prop = propinfo[objectCount + propertyCount + i];
		if(prop.name.equals(name, length)) {
			return prop.offset;
		}
	}
	return -1;
}

String ObjectInfo::getTypeDesc() const
{
	String s;
	s += ::toString(type);
	if(type == ObjectType::Array) {
		s += '[';
		s += ::toString(propinfo[0].type);
		s += ']';
	} else if(type == ObjectType::ObjectArray) {
		s += "[Object]";
	}
	return s;
}

} // namespace ConfigDB
