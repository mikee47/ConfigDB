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
const ObjectInfo PROGMEM ObjectInfo::empty{.name = fstr_empty};

size_t ObjectInfo::getOffset() const
{
	if(!parent) {
		return 0;
	}
	size_t offset = parent->getOffset();
	for(unsigned i = 0; i < parent->objectCount; ++i) {
		auto obj = parent->objinfo[i];
		if(obj == this) {
			return offset;
		}
		offset += obj->structSize;
	}
	assert(false);
	return 0;
};

String ObjectInfo::getTypeDesc() const
{
	String s;
	s += toString(type);
	if(type == ObjectType::Array) {
		s += '[';
		s += toString(propinfo[0].type);
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
	for(auto typeinfo = &getTypeinfo(); !typeinfo->isRoot(); typeinfo = typeinfo->parent) {
		if(relpath) {
			relpath += '.';
		}
		relpath += typeinfo->name;
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
	if(prop.type != PropertyType::String) {
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
	if(prop.type != PropertyType::String) {
		return false;
	}
	StringId id = getStore().stringPool.findOrAdd(value);
	memcpy(data, &id, sizeof(id));
	return true;
}

StringId Object::addString(const String& value)
{
	return getStore().stringPool.findOrAdd(value);
}

} // namespace ConfigDB
