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
	auto& typeinfo = getTypeinfo();
	if(typeinfo.path) {
		relpath += *typeinfo.path;
	}
	if(typeinfo.name) {
		if(relpath) {
			relpath += '.';
		}
		relpath += *typeinfo.name;
	}
	String path = getStore().getName();
	if(relpath) {
		path += '.';
		path += relpath;
	}
	return path;
}

} // namespace ConfigDB
