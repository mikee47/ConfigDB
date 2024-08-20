/**
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
	XX(Store)                                                                                                          \
	XX(Object)                                                                                                         \
	XX(Array)                                                                                                          \
	XX(ObjectArray)

namespace ConfigDB
{
enum class ObjectType : uint32_t {
#define XX(name) name,
	CONFIGDB_OBJECT_TYPE_MAP(XX)
#undef XX
};

String toString(ObjectType type);

struct ObjectInfo {
	ObjectType type;
	const FlashString& name;
	const ObjectInfo* parent;
	const ObjectInfo* const* objinfo;
	PGM_VOID_P defaultData;
	uint32_t structSize;
	uint32_t objectCount;
	uint32_t propertyCount;
	const PropertyInfo propinfo[];

	static const ObjectInfo empty;

	ObjectInfo(const ObjectInfo&) = delete;

	String getTypeDesc() const;

	/**
	 * @brief Get offset of this object's data relative to its parent
	 */
	size_t getOffset() const;

	/**
	 * @brief Get offset of data for a property from the start of *this* object's data
	 */
	size_t getPropertyOffset(unsigned index) const;

	int findObject(const char* name, size_t length) const;

	int findProperty(const char* name, size_t length) const;
};

} // namespace ConfigDB
