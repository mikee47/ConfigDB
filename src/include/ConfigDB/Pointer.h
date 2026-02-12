/****
 * ConfigDB/Pointer.h
 *
 * Copyright 2026 mikee47 <mike@sillyhouse.net>
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

#include "Format.h"
#include "Object.h"

namespace ConfigDB
{
class Pointer
{
public:
	Pointer(const String& string) : string(string)
	{
	}

private:
	friend class PointerContext;

	String string;
};

class PointerContext
{
public:
	static constexpr unsigned maxNesting = 16;

	bool resolve(Database& db, const Pointer& ptr);

	bool isProperty() const
	{
		return bool(property);
	}

	const Property getProperty() const
	{
		return property;
	}

	Object getObject() const
	{
		return property ? Object() : objects[nesting];
	}

	explicit operator bool() const
	{
		return database || store;
	}

	std::unique_ptr<ExportStream> createExportStream(const Format& format, const ExportOptions& options = {})
	{
		if(database) {
			return format.createExportStream(*database, options);
		}
		auto obj = getObject();
		if(obj) {
			return format.createExportStream(store, obj, options);
		}
		return nullptr;
	}

private:
	void clear()
	{
		database = nullptr;
		store = {};
		nesting = 0;
	}

	Database* database = nullptr;
	StoreRef store;
	Object objects[maxNesting];
	Property property;
	uint8_t nesting{};
};

} // namespace ConfigDB
