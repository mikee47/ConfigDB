/****
 * ConfigDB/Pointer.h
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

#include "Database.h"

namespace ConfigDB
{
class PointerContext
{
public:
	static constexpr unsigned maxNesting = 16;

	// Methods here for introspection
	// TODO

// private:
	friend class Pointer;

	PointerContext() = default;

	PointerContext(Database& db) : db(&db)
	{
	}

	PointerContext(StoreRef store, Object object) : store(store), objects{object}
	{
	}

	Database* db = nullptr;
	StoreRef store;
	Object objects[maxNesting];
	uint8_t nesting{};
};

class Pointer
{
public:
	Pointer(const String& string) : string(string)
	{
	}

	PointerContext resolve(Database& db);

private:
	String string;
};

} // namespace ConfigDB
