/**
 * ConfigDB/ArrayBase.h
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
#include "Pool.h"

namespace ConfigDB
{
/**
 * @brief Base class to provide array of properties
 */
class ArrayBase : public Object
{
public:
	using Object::Object;

	unsigned getItemCount() const
	{
		return getId() ? getArray().getCount() : 0;
	}

	bool removeItem(unsigned index)
	{
		return getArray().remove(index);
	}

	void* getItem(unsigned index)
	{
		return getArray()[index];
	}

	const void* getItem(unsigned index) const
	{
		return getArray()[index];
	}

	void addItem(const void* value)
	{
		getArray().add(value);
	}

protected:
	ArrayId& getId()
	{
		return *static_cast<ArrayId*>(getData());
	}

	ArrayId getId() const
	{
		return *static_cast<const ArrayId*>(getData());
	}

	ArrayData& getArray();
	const ArrayData& getArray() const;
};

} // namespace ConfigDB
