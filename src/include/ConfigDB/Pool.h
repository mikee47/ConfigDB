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
#include <Print.h>

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
		unsigned offset = ref - 1;
		if(offset >= strings.length()) {
			debug_e("Bad string ref %u (%u)", ref, strings.length());
			return nullptr;
		}
		return strings.c_str() + offset;
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
		assert(itemSize > 0 && itemSize <= 255);
	}

	~ArrayData()
	{
		free(buffer);
	}

	uint8_t* add(const ObjectInfo& object)
	{
		assert(itemSize == object.structSize);
		return add(object.defaultData);
	}

	uint8_t* add(const void* data = nullptr)
	{
		return checkCapacity() ? setItem(count++, data) : nullptr;
	}

	uint8_t* set(unsigned index, const void* data)
	{
		return (index < count) ? setItem(index, data) : nullptr;
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
		return setItem(index, data);
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

	size_t getCapacity() const
	{
		return capacity;
	}

	size_t getItemSize() const
	{
		return itemSize;
	}

private:
	bool checkCapacity()
	{
		if(count < capacity) {
			return true;
		}
		auto newCapacity = capacity + increment;
		auto newBuffer = realloc(buffer, getItemSize(newCapacity));
		if(!newBuffer) {
			return false;
		}
		buffer = static_cast<uint8_t*>(newBuffer);
		capacity = newCapacity;
		return true;
	}

	uint8_t* setItem(unsigned index, const void* data)
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
	uint16_t capacity{};
	uint16_t count{};
	uint8_t itemSize;
};

/**
 * @brief This pool stores array data
 *
 * An array has fixed-size items.
 */
class ArrayPool
{
public:
	ArrayId add(const ObjectInfo& object)
	{
		auto data = new ArrayData(object.structSize);
		pool.addElement(data);
		return pool.size();
	}

	ArrayId add(const PropertyInfo& prop)
	{
		auto data = new ArrayData(prop.getSize());
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

	size_t getCount() const
	{
		return pool.count();
	}

private:
	Vector<ArrayData> pool;
};

} // namespace ConfigDB
