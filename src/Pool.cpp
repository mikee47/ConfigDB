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
void* PoolData::allocate(size_t items)
{
	if(items <= space) {
		count += items;
		space -= items;
		auto ptr = getItemPtr(count - items);
		return ptr;
	}

	size_t newSpace = std::min(255U, count / 8U);
	size_t newCapacity = count + items + newSpace;

	auto newBuffer = realloc(buffer, getItemSize(newCapacity));
	if(!newBuffer) {
		return nullptr;
	}

	buffer = newBuffer;
	count += items;
	space = newSpace;
	auto ptr = getItemPtr(count - items);
	return ptr;
}

void PoolData::deallocate(size_t items)
{
	assert(items <= count);
	count -= items;
	space += items;
}

StringId StringPool::find(const char* value, size_t valueLength) const
{
	if(!buffer || !value || !valueLength) {
		return 0;
	}
	for(size_t offset = 0; offset < count;) {
		auto ptr = static_cast<char*>(buffer) + offset;
		ptr = static_cast<char*>(memmem(ptr, count - offset, value, valueLength));
		if(!ptr) {
			return 0;
		}
		offset = uintptr_t(ptr) - uintptr_t(buffer) + 1;
		if(ptr[valueLength] == '\0') {
			return offset;
		}
	}
	return 0;
}

StringId StringPool::add(const char* value, size_t valueLength)
{
	// Include NUL
	auto ptr = static_cast<char*>(allocate(valueLength + 1));
	if(!ptr) {
		return 0;
	}
	memcpy(ptr, value, valueLength);
	ptr[valueLength] = '\0';
	return 1 + ptr - static_cast<char*>(buffer);
}

bool ArrayData::remove(unsigned index)
{
	assert(index < count);
	if(index >= count) {
		return false;
	}
	if(index + 1 < count) {
		memmove(getItemPtr(index), getItemPtr(index + 1), getItemSize(count - index - 1));
	}
	deallocate(1);
	return true;
}

void* ArrayData::insert(unsigned index, const void* data)
{
	assert(index <= count);
	if(index > count) {
		return nullptr;
	}
	if(!allocate(1)) {
		return nullptr;
	}
	auto item = getItemPtr(index);
	if(index + 1 < count) {
		memmove(getItemPtr(index + 1), item, getItemSize(count - index - 1));
	}
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
		++space;
		--data;
		data->clear();
	}
	PoolData::clear();
}

ArrayId ArrayPool::add(size_t itemSize)
{
	auto ptr = allocate(1);
	if(!ptr) {
		return 0;
	}
	*static_cast<ArrayData*>(ptr) = ArrayData(itemSize);
	return count;
}

} // namespace ConfigDB
