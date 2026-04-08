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
	PointerContext() = default;

	PointerContext(const Object& obj)
	{
		objects[0] = obj;
		nesting = 1;
	}

	PointerContext(Database& db) : database(&db)
	{
	}

	PointerContext(Database& db, const Pointer& ptr)
	{
		resolve(db, ptr);
	}

	bool resolve(Database& db, const Pointer& ptr);

	bool isProperty() const
	{
		return bool(property);
	}

	const Property& getProperty() const
	{
		return property;
	}

	Object getObject() const
	{
		return nesting ? objects[nesting - 1] : Object();
	}

	Database* getDatabase() const
	{
		return database;
	}

	StoreRef getStore() const
	{
		return store;
	}

	explicit operator bool() const
	{
		return database || store;
	}

	// std::unique_ptr<ExportStream> createExportStream(const Format& format, const ExportOptions& options = {})
	// {
	// 	if(database) {
	// 		return format.createExportStream(*database, options);
	// 	}

	// 	if(property) {
	// 		// TODO: Support export a single property
	// 		return nullptr;
	// 	}

	// 	auto obj = getObject();
	// 	if(obj) {
	// 		// TODO: This won't work. We need to pass the entire context.
	// 		return format.createExportStream(store, obj, options);
	// 	}
	// 	return nullptr;
	// }

private:
	void clear()
	{
		database = nullptr;
		store = {};
		nesting = 0;
	}

	Database* database = nullptr;
	StoreRef store;
	std::unique_ptr<Object[]> objects;
	Property property;
	uint8_t nesting{};
};

} // namespace ConfigDB
