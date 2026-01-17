/****
 * ConfigDB/ObjectInfo.h
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

#pragma once

#include "PropertyInfo.h"

#define CONFIGDB_OBJECT_TYPE_MAP(XX)                                                                                   \
	XX(Object)                                                                                                         \
	XX(Array)                                                                                                          \
	XX(ObjectArray)                                                                                                    \
	XX(Union)

namespace ConfigDB
{
enum class ObjectType : uint32_t {
#define XX(name) name,
	CONFIGDB_OBJECT_TYPE_MAP(XX)
#undef XX
};

struct ObjectInfo {
	ObjectType type;
	PGM_VOID_P defaultData;
	uint32_t structSize;
	uint32_t objectCount;
	uint32_t propertyCount;
	uint32_t aliasCount;
	const PropertyInfo propinfo[];

	static const ObjectInfo empty;

	bool isArray() const
	{
		return type == ObjectType::Array || type == ObjectType::ObjectArray;
	}

	String getTypeDesc() const;

	int findObject(const char* name, size_t length) const;

	int findProperty(const char* name, size_t length) const;

	const PropertyInfo& getObject(unsigned index) const
	{
		assert(index < objectCount);
		return (index < objectCount) ? propinfo[index] : PropertyInfo::empty;
	}

	const PropertyInfo& getProperty(unsigned index) const
	{
		assert(index < propertyCount);
		return (index < propertyCount) ? propinfo[objectCount + index] : PropertyInfo::empty;
	}

private:
	int findAlias(const char* name, size_t length) const;
};

} // namespace ConfigDB

String toString(ConfigDB::ObjectType type);
