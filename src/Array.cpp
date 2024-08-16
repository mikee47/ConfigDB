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
	if(id() == 0) {
		assert(typeinfo().propertyCount == 1);
		*static_cast<ArrayId*>(data) = store.arrayPool.add(typeinfo().propinfo[0]);
	}
	return store.arrayPool[id()];
}

Property Array::getProperty(unsigned index)
{
	auto& array = getArray();
	if(index > array.getCount()) {
		return {};
	}
	// Property info contains exactly one element
	assert(typeinfo().propertyCount == 1);
	return {getStore(), typeinfo().propinfo[0], array[index]};
}

void Array::addNewItem(const char* value, size_t valueLength)
{
	assert(typeinfo().propertyCount == 1);
	auto& array = getArray();
	auto data = array[array.getCount()];
	getStore().setValueString(typeinfo().propinfo[0], data, value, valueLength);
}

} // namespace ConfigDB
