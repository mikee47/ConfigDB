/**
 * ConfigDB/DatabaseInfo.h
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

#include "PropertyInfo.h"

namespace ConfigDB
{
struct DatabaseInfo {
	const FlashString& name;
	uint32_t storeCount;
	const PropertyInfo stores[];

	const PropertyInfo& getObject(unsigned index) const
	{
		return stores[index];
	}

	int findStore(const char* name, size_t nameLength) const;
	int indexOf(const PropertyInfo& store) const;
};

} // namespace ConfigDB
