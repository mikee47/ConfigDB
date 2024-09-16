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

namespace ConfigDB
{
/**
 * @brief Base allocator, designed for good reallocation performance and low RAM footprint
 */
class PoolData
{
public:
	PoolData(size_t itemSize) : itemSize(itemSize)
	{
		assert(itemSize > 0 && itemSize <= 255);
	}

	PoolData(const PoolData& other)
	{
		*this = other;
	}

	PoolData(PoolData&& other);

	PoolData& operator=(const PoolData& other);

	size_t getCount() const
	{
		return count;
	}

	size_t getCapacity() const
	{
		return count + space;
	}

	size_t getItemSize() const
	{
		return itemSize;
	}

	size_t getItemSize(size_t count) const
	{
		return count * itemSize;
	}

	void* operator[](unsigned index)
	{
		return getItemPtr(index);
	}

	const void* operator[](unsigned index) const
	{
		return getItemPtr(index);
	}

	void clear()
	{
		free(buffer);
		buffer = nullptr;
		count = space = 0;
	}

	size_t usage() const
	{
		return getCapacity() * itemSize;
	}

protected:
	void* allocate(size_t items);
	void deallocate(size_t items);

	void* getItemPtr(unsigned index)
	{
		assert(index < count);
		return static_cast<uint8_t*>(buffer) + getItemSize(index);
	}

	const void* getItemPtr(unsigned index) const
	{
		assert(index < count);
		return static_cast<const uint8_t*>(buffer) + getItemSize(index);
	}

	void* buffer{};
	uint16_t count{0};
	uint8_t space{0};
	uint8_t itemSize;
};

/**
 * @brief Used by StringPool
 */
struct CountedString {
public:
	const char* value{};
	uint16_t length{};

	CountedString() = default;

	CountedString(const char* value, uint16_t length) : value(value), length(length)
	{
	}

	explicit CountedString(const String& s) : value(s.c_str()), length(s.length())
	{
	}

	bool operator==(const CountedString& other) const
	{
		return length == other.length && memcmp(value, other.value, length) == 0;
	}

	explicit operator bool() const
	{
		return value && length;
	}

	explicit operator String() const
	{
		return String(value, length);
	}

	/**
	 * @brief Get buffer size required to store this string
	 */
	uint16_t getStorageSize() const
	{
		return 1 + (length > 0x80) + length;
	}
};

/**
 * @brief Pool for string data
 *
 * We store all string data in a single buffer.
 * A StringId references a string item by position, and is always > 0.
 * Strings are appended but never removed.
 *
 * To support binary data we use counted strings.
 * Strings of 128 (0x80) or fewer characters use a single byte prefix containing the length.
 * Longer strings require a second byte which is combined with the first. For example:
 *
 * 0x82 0x00 0x0200
 * 0x80 	 0x0080
 * 0x81 0x00 0x0100
 *
 * Storing large amounts of binary data is not advisable.
 * Instead, configuration should store a filename or reference to some external storage.
 *
 * Note: We might consider implementing some kind of 'load on demand' feature in ConfigDB so that
 * externally stored data can be transparently fetched.
 */
class StringPool : public PoolData
{
public:
	StringPool() : PoolData(1)
	{
	}

	~StringPool()
	{
		clear();
	}

	/**
	 * @brief Search for a string
	 * @retval StringId 0 if string is not found
	 */
	StringId find(const CountedString& string) const;

	StringId add(const CountedString& string);

	StringId findOrAdd(const CountedString& string)
	{
		return find(string) ?: add(string);
	}

	CountedString operator[](StringId ref) const
	{
		unsigned offset = ref - 1;
		return offset < count ? getString(offset) : CountedString{};
	}

	const char* getBuffer() const
	{
		return static_cast<const char*>(buffer);
	}

private:
	CountedString getString(unsigned offset) const;
};

/**
 * @brief An array of fixed-sized items
 *
 * The `ArrayPool` class manages these instances.
 * This isn't type-based but rather uses an item size, so it's difficult to implement with the STL.
 */
class ArrayData : public PoolData
{
public:
	using PoolData::PoolData;

	void* insert(unsigned index, const void* value = nullptr);

	void* add(const void* value = nullptr)
	{
		return insert(getCount(), value);
	}

	bool remove(unsigned index);

	/**
	 * @brief Clear and mark this array as 'not in use' so it can be re-used
	 */
	void dispose()
	{
		clear();
		itemSize = 0;
	}
};

/**
 * @brief This pool stores array data
 *
 * An array has fixed-size items.
 */
class ArrayPool : public PoolData
{
public:
	ArrayPool() : PoolData(sizeof(ArrayData))
	{
	}

	ArrayPool(const ArrayPool& other);

	~ArrayPool()
	{
		clear();
	}

	ArrayId add(const ObjectInfo& object)
	{
		return add(object.structSize);
	}

	ArrayId add(const PropertyInfo& prop)
	{
		return add(prop.getSize());
	}

	ArrayData& operator[](ArrayId id)
	{
		return *static_cast<ArrayData*>(getItemPtr(id - 1));
	}

	const ArrayData& operator[](ArrayId id) const
	{
		return *static_cast<const ArrayData*>(getItemPtr(id - 1));
	}

	void clear();

	size_t usage() const
	{
		size_t n = PoolData::usage();
		auto data = static_cast<const ArrayData*>(buffer);
		for(unsigned i = 0; i < count; ++i) {
			n += data->usage();
			++data;
		}
		return n;
	}

private:
	ArrayId add(size_t itemSize);
};

} // namespace ConfigDB
