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

unsigned Array::getPropertyCount() const
{
	return id ? getStore().arrayPool[id].getCount() : 0;
}

void* Array::getItemPtr(unsigned index)
{
	auto& array = getStore().arrayPool[id];
	return array[index];
}

bool Array::setItemPtr(unsigned index, const void* value)
{
	auto& array = getStore().arrayPool[id];
	return array.set(index, value);
}

bool Array::addItemPtr(const void* value)
{
	auto& object = getTypeinfo();
	auto& store = getStore();
	if(id == 0) {
		assert(object.propertyCount == 1);
		id = store.arrayPool.add(object.propinfo[0]);
	}
	auto& array = store.arrayPool[id];
	return array.add(value);
}

bool Array::removeItem(unsigned index)
{
	return getStore().arrayPool[id].remove(index);
}

} // namespace ConfigDB
