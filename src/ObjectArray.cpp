/**
 * ConfigDB/ObjectArray.cpp
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

#include "include/ConfigDB/ObjectArray.h"
#include "include/ConfigDB/Store.h"

namespace ConfigDB
{
ObjectArray::ObjectArray(Store& store, const ObjectInfo& typeinfo)
	: Object(), id(store.getObjectData<ArrayId>(typeinfo))
{
}

unsigned ObjectArray::getObjectCount() const
{
	return id ? getStore().getObjectArray(id).getCount() : 0;
}

void* ObjectArray::getObjectDataPtr(unsigned index)
{
	auto& store = getStore();
	if(id == 0) {
		id = store.objectArrayPool.add();
	}
	auto& array = store.objectArrayPool[id];
	if(index == array.getCount()) {
		auto& items = getTypeinfo().objinfo->valueAt(0);
		array.add(items.getStructSize(), items.defaultData);
	}
	return array[index].get();
}

} // namespace ConfigDB
