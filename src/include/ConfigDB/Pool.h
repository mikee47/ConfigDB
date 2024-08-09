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
class PoolData : public std::unique_ptr<uint8_t[]>
{
public:
	PoolData(uint16_t size) : unique_ptr(new uint8_t[size]), size(size)
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
	PoolData& add(PGM_VOID_P defaultData, size_t size)
	{
		auto data = new PoolData(size);
		memcpy_P(data->get(), defaultData, size);
		pool.addElement(data);
		return *data;
	}

	PoolData& operator[](unsigned index)
	{
		return pool[index];
	}

private:
	Vector<PoolData> pool;
};

/**
 * @brief This pool stores array data
 *
 * An array has fixed-size items.
 * These can be properties or Object references.
 */
class ArrayPool : public Vector<PoolData>
{
};

} // namespace ConfigDB
