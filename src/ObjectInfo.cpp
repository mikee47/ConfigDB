/**
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

namespace ConfigDB
{
const ObjectInfo PROGMEM ObjectInfo::empty{.name = fstr_empty};

String toString(ObjectType type)
{
	switch(type) {
#define XX(name)                                                                                                       \
	case ObjectType::name:                                                                                             \
		return F(#name);
		CONFIGDB_OBJECT_TYPE_MAP(XX)
#undef XX
	}
	return nullptr;
}

size_t ObjectInfo::getOffset() const
{
	if(!parent) {
		return 0;
	}
	size_t offset{0};
	for(unsigned i = 0; i < parent->objectCount; ++i) {
		auto obj = parent->objinfo[i];
		if(obj == this) {
			return offset;
		}
		offset += obj->structSize;
	}
	assert(false);
	return 0;
}

size_t ObjectInfo::getPropertyOffset(unsigned index) const
{
	size_t offset{0};
	for(unsigned i = 0; i < objectCount; ++i) {
		offset += objinfo[i]->structSize;
	}
	for(unsigned i = 0; i < propertyCount; ++i) {
		if(i == index) {
			return offset;
		}
		offset += propinfo[i].getSize();
	}
	assert(false);
	return 0;
}

int ObjectInfo::findObject(const char* name, size_t length) const
{
	for(unsigned i = 0; i < objectCount; ++i) {
		if(objinfo[i]->name.equals(name, length)) {
			return i;
		}
	}
	return -1;
}

int ObjectInfo::findProperty(const char* name, size_t length) const
{
	for(unsigned i = 0; i < propertyCount; ++i) {
		if(propinfo[i].name.equals(name, length)) {
			return i;
		}
	}
	return -1;
}

String ObjectInfo::getTypeDesc() const
{
	String s;
	s += toString(type);
	if(type == ObjectType::Array) {
		s += '[';
		s += toString(propinfo[0].type);
		s += ']';
	} else if(type == ObjectType::ObjectArray) {
		s += "[Object]";
	}
	return s;
}

} // namespace ConfigDB
