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
	increment = std::max(increment, 8U / itemSize);
	size_t newCapacity = std::max(required, capacity + increment);
	auto newBuffer = realloc(buffer, getItemSize(newCapacity));
	if(!newBuffer) {
		return false;
	}
	buffer = newBuffer;
	capacity = newCapacity;
	return true;
}

StringId StringPool::find(const char* value, size_t valueLength) const
{
	if(!buffer || !value || !valueLength) {
		return 0;
	}
	for(size_t offset = 0;;) {
		auto ptr = static_cast<char*>(buffer) + offset;
		ptr = static_cast<char*>(memmem(ptr, count - offset, value, valueLength));
		if(!ptr) {
			return 0;
		}
		offset = uintptr_t(ptr) - uintptr_t(buffer) + 1;
		if(ptr[valueLength + 1] == '\0') {
			return offset;
		}
	}
}

StringId StringPool::add(const char* value, size_t valueLength)
{
	// Include NUL
	if(!ensureCapacity(count + valueLength + 1)) {
		return 0;
	}
	auto id = 1 + count;
	auto ptr = static_cast<char*>(buffer) + count;
	memcpy(ptr, value, valueLength);
	ptr[valueLength] = '\0';
	count += valueLength + 1;
	return id;
}

bool ArrayData::remove(unsigned index)
{
	assert(index < count);
	if(index >= count) {
		return false;
	}
	memmove(getItemPtr(index), getItemPtr(index + 1), getItemSize(count - index - 1));
	--count;
	return true;
}

void* ArrayData::insertItem(unsigned index, const void* data)
{
	assert(index <= count);
	if(index > count) {
		return nullptr;
	}
	if(!ensureCapacity(count + 1)) {
		return nullptr;
	}
	if(index < count) {
		memmove(getItemPtr(index + 1), getItemPtr(index), getItemSize(count - index));
	}
	++count;
	return setItem(index, data);
}

void* ArrayData::setItem(unsigned index, const void* data)
{
	assert(index < count);
	if(index >= count) {
		return nullptr;
	}
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
