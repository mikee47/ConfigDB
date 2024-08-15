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

ArrayData& Array::getArray()
{
	auto& store = getStore();
	if(id == 0) {
		auto& object = getTypeinfo();
		assert(object.propertyCount == 1);
		id = store.arrayPool.add(object.propinfo[0]);
	}
	return store.arrayPool[id];
}

StringId Array::addString(const String& value)
{
	return getStore().stringPool.findOrAdd(value.c_str(), value.length());
}

Property Array::getProperty(unsigned index)
{
	auto& array = getArray();
	if(index > array.getCount()) {
		return {};
	}
	// Property info contains exactly one element
	auto& typeinfo = getTypeinfo();
	assert(typeinfo.propertyCount == 1);
	return {*this, typeinfo.propinfo[0], array[index]};
}

bool Array::addNewItem(const char* value, size_t valueLength)
{
	auto& typeinfo = getTypeinfo();
	assert(typeinfo.propertyCount == 1);
	auto& array = getArray();
	auto data = array[array.getCount()];
	return setPropertyValue(typeinfo.propinfo[0], data, value, valueLength);
}

} // namespace ConfigDB
