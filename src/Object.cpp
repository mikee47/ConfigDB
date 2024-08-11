/**
 * ConfigDB/Object.cpp
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

#include "include/ConfigDB/Object.h"
#include "include/ConfigDB/Store.h"

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
String ObjectInfo::getTypeDesc() const
{
	String s;
	auto type = getType();
	s += toString(type);
	if(type == ObjectType::Array) {
		s += '[';
		s += toString(propinfo->valueAt(0).getType());
		s += ']';
	} else if(type == ObjectType::ObjectArray) {
		s += "[]";
	}
	return s;
}

bool Object::commit()
{
	return getStore().commit();
}

Database& Object::getDatabase()
{
	return getStore().getDatabase();
}

String Object::getPath() const
{
	String relpath;
	auto typeinfo = &getTypeinfo();
	while(typeinfo) {
		if(relpath && typeinfo->name) {
			relpath += '.';
		}
		if(typeinfo->name) {
			relpath += *typeinfo->name;
		}
		typeinfo = typeinfo->parent;
	}
	String path = getStore().getName();
	if(relpath) {
		path += '.';
		path += relpath;
	}
	return path;
}

String Object::getPropertyValue(const PropertyInfo& prop, const void* data) const
{
	if(prop.getType() != PropertyType::String) {
		return nullptr;
	}
	auto id = *reinterpret_cast<const StringId*>(data);
	if(id == 0) {
		return prop.getDefaultValue();
	}
	return getStore().stringPool[id];
}

bool Object::setPropertyValue(const PropertyInfo& prop, void* data, const String& value)
{
	if(prop.getType() != PropertyType::String) {
		return false;
	}
	auto& id = *reinterpret_cast<StringId*>(data);
	id = getStore().stringPool.findOrAdd(value);
	return true;
}

StringId Object::addString(const String& value)
{
	return getStore().stringPool.findOrAdd(value);
}

} // namespace ConfigDB
