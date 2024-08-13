/**
 * ConfigDB/Json/Store.h
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

#include "../Store.h"

namespace ConfigDB::Json
{
class Store : public ConfigDB::Store
{
public:
	using ConfigDB::Store::Store;

	bool commit() override
	{
		return save();
	}

	String getFilename() const
	{
		return getPath() + ".json";
	}

	size_t printTo(Print& p, unsigned nesting) const override;

	void printObjectTo(const ObjectInfo& object, const void* data, unsigned nesting, Print& p) const;
	void printArrayTo(const ObjectInfo& object, ArrayId id, unsigned nesting, Print& p) const;
	void printObjectArrayTo(const ObjectInfo& object, ArrayId id, unsigned nesting, Print& p) const;

protected:
	/**
	 * Loading starts with default data loaded from schema, which is then updated during load.
	 * Failure indicates corrupt JSON file, but any readable data is available.
	 */
	bool load();
	bool save();

	String getValueJson(const PropertyInfo& info, const void* data) const;
};

template <class ClassType> using StoreTemplate = ConfigDB::StoreTemplate<Store, ClassType>;

} // namespace ConfigDB::Json
