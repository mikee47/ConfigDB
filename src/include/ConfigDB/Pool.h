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

	PoolData(const PoolData&) = delete;

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
		return static_cast<uint8_t*>(buffer) + getItemSize(index);
	}

	const void* getItemPtr(unsigned index) const
	{
		return static_cast<const uint8_t*>(buffer) + getItemSize(index);
	}

	void* buffer{};
	uint16_t count{};
	uint8_t space{};
	uint8_t itemSize;
};

/**
 * @brief Pool for string data
 *
 * We store all string data in a single String object, NUL-separated.
 * A StringId is the offset from the start of that object.
 * Strings are appended but never removed.
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
	StringId find(const char* value, size_t valueLength) const;

	StringId add(const char* value, size_t valueLength);

	StringId findOrAdd(const char* value, size_t valueLength)
	{
		return find(value, valueLength) ?: add(value, valueLength);
	}

	const char* operator[](StringId ref) const
	{
		unsigned offset = ref - 1;
		assert(offset < count);
		return static_cast<const char*>(buffer) + offset;
	}

	const char* getBuffer()
	{
		return static_cast<const char*>(buffer);
	}
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

	template <typename T> typename std::enable_if<std::is_integral<T>::value, void*>::type add(T value)
	{
		assert(itemSize == sizeof(T));
		return addItem(&value);
	}

	void* add(const ObjectInfo& object)
	{
		assert(itemSize == object.structSize);
		return addItem(object.defaultData);
	}

	template <typename T> typename std::enable_if<std::is_integral<T>::value, void*>::type set(unsigned index, T value)
	{
		assert(itemSize == sizeof(T));
		return setItem(index, &value);
	}

	template <typename T>
	typename std::enable_if<std::is_integral<T>::value, void*>::type insert(unsigned index, T value)
	{
		assert(itemSize == sizeof(T));
		return insertItem(index, &value);
	}

	void* insert(unsigned index, const ObjectInfo& object)
	{
		assert(itemSize == object.structSize);
		return insertItem(index, object.defaultData);
	}

	bool remove(unsigned index);

	void* operator[](unsigned index)
	{
		if(index == count) {
			return addItem(nullptr);
		}
		if(index < count) {
			return getItemPtr(index);
		}
		return nullptr;
	}

	const void* operator[](unsigned index) const
	{
		return (index < count) ? getItemPtr(index) : nullptr;
	}

private:
	void* addItem(const void* data)
	{
		return insertItem(count, data);
	}
	void* setItem(unsigned index, const void* data);
	void* insertItem(unsigned index, const void* data);
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
		auto index = id - 1;
		assert(index < count);
		return *static_cast<ArrayData*>(getItemPtr(index));
	}

	const ArrayData& operator[](ArrayId id) const
	{
		auto index = id - 1;
		assert(index < count);
		return *static_cast<const ArrayData*>(getItemPtr(index));
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
