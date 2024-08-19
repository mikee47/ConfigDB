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
ArrayData& Array::getArray()
{
	auto& store = getStore();
	auto& id = getId();
	if(id == 0) {
		assert(typeinfo().propertyCount == 1);
		id = store.arrayPool.add(typeinfo().propinfo[0]);
	}
	return store.arrayPool[id];
}

Property Array::getProperty(unsigned index)
{
	assert(typeinfo().propertyCount == 1);
	return {getStore(), typeinfo().propinfo[0], getArray()[index]};
}

void Array::addNewItem(const char* value, size_t valueLength)
{
	assert(typeinfo().propertyCount == 1);
	getStore().setValueString(typeinfo().propinfo[0], getArray().add(), value, valueLength);
}

} // namespace ConfigDB
