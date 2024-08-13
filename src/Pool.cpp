/**
 * ConfigDB/Pool.cpp
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

#include "include/ConfigDB/Pool.h"

namespace ConfigDB
{
bool PoolData::ensureCapacity(size_t required)
{
	if(required < capacity) {
		return true;
	}
	size_t increment = std::max(4, capacity / 4);
	size_t newCapacity = std::max(required, capacity + increment);
	auto newBuffer = realloc(buffer, getItemSize(newCapacity));
	if(!newBuffer) {
		return false;
	}
	buffer = newBuffer;
	capacity = newCapacity;
	return true;
}

StringId StringPool::find(const String& value) const
{
	if(!buffer) {
		return 0;
	}
	auto ptr = memmem(buffer, count, value.c_str(), value.length() + 1);
	if(!ptr) {
		return 0;
	}
	auto offset = uintptr_t(ptr) - uintptr_t(buffer);
	return 1 + offset;
}

StringId StringPool::add(const String& value)
{
	auto valueLength = value.length() + 1; // Include NUL
	if(!ensureCapacity(count + valueLength)) {
		return 0;
	}
	memcpy(static_cast<char*>(buffer) + count, value.c_str(), valueLength);
	auto id = 1 + count;
	count += valueLength;
	return id;
}

void* ArrayData::add(const void* data)
{
	return ensureCapacity(count + 1) ? setItem(count++, data) : nullptr;
}

bool ArrayData::remove(unsigned index)
{
	if(index >= count) {
		return false;
	}
	memmove(getItemPtr(index), getItemPtr(index + 1), getItemSize(count - index - 1));
	--count;
	return true;
}

void* ArrayData::insert(unsigned index, const void* data)
{
	if(index > count || !ensureCapacity(count + 1)) {
		return nullptr;
	}
	memmove(getItemPtr(index + 1), getItemPtr(index), getItemSize(count - index - 1));
	++count;
	return setItem(index, data);
}

void* ArrayData::setItem(unsigned index, const void* data)
{
	auto item = getItemPtr(index);
	if(data) {
		memcpy_P(item, data, itemSize);
	} else {
		memset(item, 0, itemSize);
	}
	return item;
}

void ArrayPool::clear()
{
	auto data = static_cast<ArrayData*>(buffer) + count;
	while(count--) {
		--data;
		data->clear();
	}
	PoolData::clear();
}

ArrayId ArrayPool::add(size_t itemSize)
{
	if(!ensureCapacity(count + 1)) {
		return 0;
	}
	auto ptr = getItemPtr(count++);
	*static_cast<ArrayData*>(ptr) = ArrayData(itemSize);
	return count;
}

} // namespace ConfigDB
