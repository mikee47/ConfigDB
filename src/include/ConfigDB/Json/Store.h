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

	String getFilename() const
	{
		String path = getFilePath();
		path += _F(".json");
		return path;
	}

	size_t printObjectTo(const Object& object, const FlashString* name, unsigned nesting, Print& p) const override;

	bool load() override;
	bool save() override;

protected:
	String getValueJson(const PropertyInfo& info, const void* data) const;
};

} // namespace ConfigDB::Json
