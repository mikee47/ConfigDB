/****
 * ConfigDB/StoreRef.h
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

#include <memory>

namespace ConfigDB
{
class Store;

class StoreRef : public std::shared_ptr<Store>
{
public:
	StoreRef() = default;

	StoreRef(std::shared_ptr<Store> store) : shared_ptr(store)
	{
	}

	~StoreRef();

	explicit operator bool() const;

private:
	using shared_ptr::reset;
};

class StoreUpdateRef : public StoreRef
{
public:
	StoreUpdateRef() = default;

	StoreUpdateRef(const StoreRef& store)
	{
		*this = store;
	}

	StoreUpdateRef(const StoreUpdateRef& store) : StoreUpdateRef(static_cast<const StoreRef&>(store))
	{
	}

	~StoreUpdateRef();

	StoreUpdateRef& operator=(const StoreRef& other);

	StoreUpdateRef& operator=(const StoreUpdateRef& other)
	{
		return operator=(static_cast<const StoreRef&>(other));
	}
};

} // namespace ConfigDB
