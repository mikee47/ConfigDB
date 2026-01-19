/****
 * ConfigDB/DatabaseInfo.cpp
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

#include "include/ConfigDB/DatabaseInfo.h"

namespace ConfigDB
{
int DatabaseInfo::findStore(const char* name, size_t nameLength) const
{
	for(unsigned i = 0; i < storeCount; ++i) {
		auto& store = stores[i];
		if(store.name.equals(name, nameLength)) {
			return i;
		}
	}
	return -1;
}

int DatabaseInfo::indexOf(const PropertyInfo& store) const
{
	for(unsigned i = 0; i < storeCount; ++i) {
		if(&store == &stores[i]) {
			return i;
		}
	}
	return -1;
}

} // namespace ConfigDB
