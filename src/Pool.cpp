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
PoolData::PoolData(PoolData&& other)
{
	std::swap(buffer, other.buffer);
	std::swap(count, other.count);
	std::swap(space, other.space);
	itemSize = other.itemSize;
}

PoolData& PoolData::operator=(const PoolData& other)
{
	if(this == &other) {
		return *this;
	}

	clear();
	count = other.count;
	space = other.space;
	itemSize = other.itemSize;
	if(getCapacity()) {
		buffer = malloc(usage());
		memcpy(buffer, other.buffer, count * itemSize);
	}
	return *this;
}

bool PoolData::ensureCapacity(size_t capacity)
{
	if(count + space >= capacity) {
		return true;
	}

	auto newBuffer = realloc(buffer, getItemSize(capacity));
	if(!newBuffer) {
		return false;
	}

	buffer = newBuffer;
	space = capacity - count;
	return true;
}

void* PoolData::allocate(size_t itemCount)
{
	if(itemCount <= space) {
		count += itemCount;
		space -= itemCount;
		auto ptr = getItemPtr(count - itemCount);
		return ptr;
	}

	auto capacity = count + itemCount + std::min(255U, count / 8U);

	if(!ensureCapacity(capacity)) {
		return nullptr;
	}

	count += itemCount;
	space -= itemCount;
	return getItemPtr(count - itemCount);
}

void PoolData::deallocate(size_t itemCount)
{
	assert(itemCount <= count);
	count -= itemCount;
	space += itemCount;
}

CountedString StringPool::getString(unsigned offset) const
{
	auto ptr = static_cast<const char*>(buffer) + offset;
	uint16_t len = uint8_t(*ptr++);
	if(len > 0x80) {
		len = ((len & 0x7f) << 8) | uint8_t(*ptr++);
	}
	return {ptr, len};
}

StringId StringPool::find(const CountedString& string) const
{
	if(!buffer || !string) {
		return 0;
	}
	for(size_t offset = 0; offset < count;) {
		auto cs = getString(offset);
		if(string == cs) {
			return 1 + offset;
		}
		offset += cs.getStorageSize();
	}
	return 0;
}

StringId StringPool::add(const CountedString& string)
{
	auto ptr = static_cast<uint8_t*>(allocate(string.getStorageSize()));
	if(!ptr) {
		return 0;
	}
	auto id = 1 + ptr - static_cast<const uint8_t*>(buffer);
	if(string.length > 0x80) {
		*ptr++ = 0x80 | (string.length >> 8);
	}
	*ptr++ = string.length & 0xff;
	memcpy(ptr, string.value, string.length);
	return id;
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

void* ArrayData::insert(unsigned index, const void* data, size_t itemCount)
{
	assert(index <= count);
	if(index > count) {
		return nullptr;
	}
	if(!allocate(itemCount)) {
		return nullptr;
	}
	auto item = getItemPtr(index);
	if(index + itemCount < count) {
		memmove(getItemPtr(index + itemCount), item, getItemSize(count - index - itemCount));
	}
	if(data) {
		memcpy_P(item, data, itemSize * itemCount);
	} else {
		memset(item, 0, itemSize * itemCount);
	}
	return item;
}

ArrayPool::ArrayPool(const ArrayPool& other) : PoolData(other)
{
	if(!buffer) {
		return;
	}

	// We've copied a bunch of pointers from 'other' - clear that
	memset(buffer, 0, PoolData::usage());

	auto src = static_cast<const ArrayData*>(other.buffer);
	auto dst = static_cast<ArrayData*>(buffer);
	for(unsigned i = 0; i < count; ++i) {
		dst[i] = src[i];
	}
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
	// See if there's a free slot
	for(unsigned id = 1; id <= count; ++id) {
		auto& arr = (*this)[id];
		if(arr.getItemSize() == 0) {
			debug_d("[CFGDB] ArrayPool re-use #%u", id);
			arr = ArrayData(itemSize);
			return id;
		}
	}

	auto ptr = allocate(1);
	if(!ptr) {
		return 0;
	}
	memset(ptr, 0, sizeof(ArrayData));
	*static_cast<ArrayData*>(ptr) = ArrayData(itemSize);
	return count;
}

} // namespace ConfigDB
