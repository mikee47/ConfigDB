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

	const char* operator[](StringId ref)
	{
		return ref ? &strings[ref] : nullptr;
	}

	size_t printTo(Print& p) const
	{
		String s(strings);
		s.replace('\0', ';');
		return p.print(s);
	}

private:
	String strings;
};

/**
 * @brief Custom vector clas
 *
 * This class differs from a Vector or std::vector in the following ways:
 *  - no insert
 *  - no re-ordering
 *  - no item removal
 *
 * When an item is added it is referenced by index (slot).
 *
 * Supported operations:
 * 	- add item
 *  - clear item: de-allocates item storage and sets to 'null'
 *  - clear vector
 *
 * Object data is stored as simple uint8_t[], size is fixed
 * Array data is stored as Vector, each item is of fixed size
 * This incldues ObjectArray whose items are references into the Object pool.
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
	ObjectId add(size_t size)
	{
		auto data = new ObjectData(size);
		memset(data->get(), 0, size);
		pool.addElement(data);
		return pool.size();
	}

	ObjectId add(PGM_VOID_P defaultData, size_t size)
	{
		auto data = new ObjectData(size);
		memcpy_P(data->get(), defaultData, size);
		pool.addElement(data);
		return pool.size();
	}

	ObjectData& operator[](ObjectId id)
	{
		return pool[id - 1];
	}

private:
	Vector<ObjectData> pool;
};

class ItemPool
{
public:
	ItemPool(uint16_t itemSize) : itemSize(itemSize)
	{
	}

	uint8_t* add(const void* data = nullptr)
	{
		if(capacity == count) {
			auto newCapacity = capacity + 16;
			auto newBuffer = realloc(buffer, newCapacity);
			if(!newBuffer) {
				return nullptr;
			}
			buffer = static_cast<uint8_t*>(newBuffer);
			capacity = newCapacity;
		}
		auto item = &buffer[count * itemSize];
		if(data) {
			memcpy(item, data, itemSize);
		} else {
			memset(item, 0, itemSize);
		}
		++count;
		return item;
	}

	uint8_t* operator[](unsigned index)
	{
		return (index < count) ? &buffer[index * itemSize] : nullptr;
	}

private:
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
		auto pool = new ItemPool(itemSize);
		pools.addElement(pool);
		return pools.size();
	}

	ItemPool& operator[](ArrayId id)
	{
		return pools[id - 1];
	}

private:
	Vector<ItemPool> pools;
};

/**
 * @brief This pool stores object pools
 */
class ObjectArrayPool
{
public:
	ArrayId add()
	{
		auto pool = new ObjectPool();
		pools.addElement(pool);
		return pools.size();
	}

	ObjectPool& operator[](ArrayId id)
	{
		return pools[id - 1];
	}

private:
	Vector<ObjectPool> pools;
};

} // namespace ConfigDB
