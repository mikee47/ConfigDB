/**
 * ConfigDB/Array.h
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
 * @brief An array is basically an object with numeric keys
 */
class Array : public Object
{
public:
	using Object::Object;
};

/**
 * @brief Array class template for use by code generator
 */
template <class StoreType, class ClassType> class ArrayTemplate : public Array
{
public:
	ArrayTemplate(std::shared_ptr<StoreType> store, const String& path) : Array(path), store(store)
	{
	}

	std::shared_ptr<Store> getStore() const override
	{
		return store;
	}

	size_t printTo(Print& p) const
	{
		return getStore()->printArrayTo(*this, p);
	}

protected:
	std::shared_ptr<StoreType> store;
};

} // namespace ConfigDB
