/**
 * ConfigDB/Pool.h
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

#pragma once

#include "Object.h"
#include <WVector.h>

#include <debug_progmem.h>

namespace ConfigDB
{
/**
 * @brief Pool for string data
 *
 * We store all string data in a single String object, NUL-separated.
 * A StringId is the offset from the start of that object.
 * Strings are appended but never removed.
 */
class StringPool
{
public:
	/**
	 * @brief Search for a string
	 * @retval StringId 0 if string is not found
	 */
	StringId find(const String& value)
	{
		return 1 + strings.indexOf(value.c_str(), value.length() + 1);
	}

	StringId add(const String& value)
	{
		auto offset = strings.length();
		strings.concat(value.c_str(), value.length() + 1);
		return 1 + offset;
	}

	StringId findOrAdd(const String& value)
	{
		return find(value) ?: add(value);
	}

	const char* operator[](StringId ref) const
	{
		if(ref >= strings.length()) {
			debug_e("Bad string ref %u (%u)", ref, strings.length());
			return nullptr;
		}
		return ref ? (strings.c_str() + ref) : nullptr;
	}

	size_t printTo(Print& p) const
	{
		String s(strings);
		s.replace('\0', ';');
		return p.print(s);
	}

	void clear()
	{
		strings = nullptr;
	}

private:
	String strings;
};

/**
 * @brief Contains object data as a single heap allocation
 */
class ObjectData : public std::unique_ptr<uint8_t[]>
{
public:
	ObjectData(uint16_t size) : unique_ptr(new uint8_t[size]), size(size)
	{
	}

	uint16_t getSize() const
	{
		return size;
	}

private:
	uint16_t size;
};

/**
 * @brief This pool stores object data
 */
class ObjectPool
{
public:
	ObjectId add(size_t size, PGM_VOID_P defaultData = nullptr)
	{
		auto data = new ObjectData(size);
		if(defaultData) {
			memcpy_P(data->get(), defaultData, size);
		} else {
			memset(data->get(), 0, size);
		}
		pool.addElement(data);
		return pool.size();
	}

	ObjectData& operator[](ObjectId id)
	{
		return pool[id - 1];
	}

	const ObjectData& operator[](ObjectId id) const
	{
		return pool[id - 1];
	}

	void clear()
	{
		pool.clear();
	}

private:
	Vector<ObjectData> pool;
};

/**
 * @brief An array of fixed-sized items
 *
 * The `ArrayPool` class manages these instances.
 * This isn't type-based but rather uses an item size, so it's difficult to implement with the STL.
 */
class ArrayData
{
public:
	ArrayData(uint16_t itemSize) : itemSize(itemSize)
	{
	}

	~ArrayData()
	{
		free(buffer);
	}

	uint8_t* add(const void* data = nullptr)
	{
		return checkCapacity() ? newItem(count++, data) : nullptr;
	}

	bool remove(unsigned index)
	{
		if(index >= count) {
			return false;
		}
		memmove(getItemPtr(index), getItemPtr(index + 1), getItemSize(count - index - 1));
		--count;
		return true;
	}

	uint8_t* insert(unsigned index, const void* data = nullptr)
	{
		if(!checkCapacity()) {
			return nullptr;
		}
		memmove(getItemPtr(index + 1), getItemPtr(index), getItemSize(count - index - 1));
		++count;
		return newItem(index, data);
	}

	uint8_t* operator[](unsigned index)
	{
		return (index < count) ? getItemPtr(index) : nullptr;
	}

	const uint8_t* operator[](unsigned index) const
	{
		return const_cast<ArrayData*>(this)->operator[](index);
	}

	size_t getCount() const
	{
		return count;
	}

private:
	bool checkCapacity()
	{
		if(count < capacity) {
			return true;
		}
		auto newCapacity = capacity + getItemSize(increment);
		auto newBuffer = realloc(buffer, newCapacity);
		if(!newBuffer) {
			return false;
		}
		buffer = static_cast<uint8_t*>(newBuffer);
		capacity = newCapacity;
		return true;
	}

	uint8_t* newItem(unsigned index, const void* data)
	{
		auto item = getItemPtr(index);
		if(data) {
			memcpy(item, data, itemSize);
		} else {
			memset(item, 0, itemSize);
		}
		return item;
	}

	uint8_t* getItemPtr(unsigned index)
	{
		return &buffer[index * itemSize];
	}

	size_t getItemSize(size_t count) const
	{
		return count * itemSize;
	}

	static const size_t increment = 16;
	uint8_t* buffer{};
	size_t capacity{};
	size_t count{};
	uint16_t itemSize;
};

/**
 * @brief This pool stores array data
 *
 * An array has fixed-size items.
 */
class ArrayPool
{
public:
	ArrayId add(size_t itemSize)
	{
		auto data = new ArrayData(itemSize);
		pool.addElement(data);
		return pool.size();
	}

	ArrayData& operator[](ArrayId id)
	{
		return pool[id - 1];
	}

	const ArrayData& operator[](ArrayId id) const
	{
		return pool[id - 1];
	}

	void clear()
	{
		pool.clear();
	}

private:
	Vector<ArrayData> pool;
};

/**
 * @brief This pool stores object pools
 *
 * Accessor objects (the generated classes) contain a reference to object data,
 * so objects cannot be reallocated after creation.
 *
 * This class differs from a Vector or std::vector in the following ways:
 *  - no insert
 *  - no re-ordering
 *  - no item removal
 *
 * When an item is added it is referenced by index (slot).
 *
 * Supported operations:
 * 	- add item: use first free slot or add some more
 *  - clear item: de-allocates item storage and sets to 'null'
 *  - clear vector
 *
 * Object data is stored as simple uint8_t[], size is fixed
 * Array data is stored as Vector, each item is of fixed size
 * This incldues ObjectArray whose items are references into the Object pool.
 */
class ObjectArrayPool
{
public:
	~ObjectArrayPool()
	{
		clear();
	}

	ArrayId add()
	{
		ArrayId id = 1;
		for(auto& pool : pools) {
			if(pool == nullptr) {
				pool = new ObjectPool();
				return id;
			}
			++id;
		}
		pools.add(new ObjectPool());
		return id;
	}

	/**
	 * @brief Delete ObjectPool with given id and release slot
	 */
	void clearSlot(ArrayId id)
	{
		auto& pool = pools[id - 1];
		delete pool;
		pool = nullptr;
	}

	ObjectPool& operator[](ArrayId id)
	{
		auto pool = pools[id - 1];
		assert(pool != nullptr);
		return *pool;
	}

	const ObjectPool& operator[](ArrayId id) const
	{
		return const_cast<ObjectArrayPool*>(this)->operator[](id);
	}

	void clear()
	{
		for(auto& pool : pools) {
			delete pool;
			pool = nullptr;
		}
		pools.clear();
	}

private:
	Vector<ObjectPool*> pools;
};

} // namespace ConfigDB
