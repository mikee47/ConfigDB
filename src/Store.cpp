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

#include "include/ConfigDB/Database.h"

namespace ConfigDB
{
String Store::getPath() const
{
	String path = db.getPath();
	path += '/';
	path += name ?: F("_root");
	return path;
}

void* Store::getObjectDataPtr(const ObjectInfo& object)
{
	// Type information needs offset from start of parent object
	// Also require parent typeinfo
	auto& data = objectPool[1];
	size_t offset{0};

	return data.get() + object.getOffset();
}

void* Store::getObjectArrayDataPtr(ArrayId arrayId, unsigned index)
{
	assert(false);
	// TODO
	return nullptr;
}

} // namespace ConfigDB
