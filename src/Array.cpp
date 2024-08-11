/**
 * ConfigDB/Array.cpp
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

#include "include/ConfigDB/Array.h"
#include "include/ConfigDB/Store.h"

namespace ConfigDB
{
Array::Array(Store& store, const ObjectInfo& typeinfo) : Object(), id(store.getObjectData<ArrayId>(typeinfo))
{
}

const void* Array::getItemPtr(unsigned index) const
{
	auto& object = getTypeinfo();
	auto& prop = *object.propinfo->data();
	auto itemSize = prop.getSize();
	auto& array = getStore().arrayPool[id];
	return array[index];
}

bool Array::setItemPtr(unsigned index, const void* value)
{
	auto& object = getTypeinfo();
	auto& prop = *object.propinfo->data();
	auto itemSize = prop.getSize();
	auto& array = getStore().arrayPool[id];
	memcpy(array[index], value, itemSize);
	return true;
}

bool Array::addItemPtr(const void* value)
{
	auto& object = getTypeinfo();
	auto& prop = *object.propinfo->data();
	auto itemSize = prop.getSize();
	auto& store = getStore();
	if(id == 0) {
		id = store.arrayPool.add(itemSize);
	}
	auto& array = store.arrayPool[id];
	array.add(value);
	return true;
}

} // namespace ConfigDB
