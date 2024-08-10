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
	Store(Database& db, const String& name) : ConfigDB::Store(db, name)
	{
		load();
	}

	bool commit() override
	{
		return save();
	}

	String getFilename() const
	{
		return getPath() + ".json";
	}

	size_t printTo(Print& p) const override;

	template <class T> static size_t printObjectTo(T& obj, Format format, Print& p)
	{
		// switch(format) {
		// case Format::Compact:
		// 	return serializeJson(obj, p);
		// case Format::Pretty:
		// 	return serializeJsonPretty(obj, p);
		// }
		return 0;
	}

private:
	bool load();
	bool save();
};

template <class ClassType> using StoreTemplate = ConfigDB::StoreTemplate<Store, ClassType>;

} // namespace ConfigDB::Json
