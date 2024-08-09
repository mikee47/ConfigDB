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
#include <cstdint>
#include <memory>

namespace ConfigDB
{
/**
 * @brief Pool for string data
 *
 * We store all string data in a single String object, NUL-separated.
 * A StringRef is the offset from the start of that object.
 * Strings are appended but never removed.
 */
class StringPool
{
public:
	/**
	 * @brief Search for a string
	 * @retval StringRef 0 if string is not found
	 */
	StringRef find(const String& value)
	{
		return 1 + strings.indexOf(value.c_str(), value.length() + 1);
	}

	StringRef add(const String& value)
	{
		auto offset = strings.length();
		strings.concat(value.c_str(), value.length() + 1);
		return 1 + offset;
	}

	StringRef findOrAdd(const String& value)
	{
		return find(value) ?: add(value);
	}

	const char* operator[](StringRef ref)
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
class Vector
{
public:
private:
	std::unique_ptr<uint8_t> data;
};

/**
 * @brief This pool stores object data
 */
class ObjectPool : public Vector
{
};

/**
 * @brief This pool stores array data
 */
class ArrayPool : public Vector
{
};

} // namespace ConfigDB
