/****
 * ConfigDB/Pointer.cpp
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

#include "include/ConfigDB/Pointer.h"
#include "include/ConfigDB/Database.h"
#include <Data/CStringArray.h>

namespace ConfigDB
{
bool PointerContext::resolve(Database& db, const Pointer& ptr)
{
	clear();

	CStringArray csa;
	{
		String tmp = ptr.string;
		if(tmp[0] == '/') {
			tmp.remove(0, 1);
		}
		tmp.replace('/', '\0');
		csa = std::move(tmp);
	}
	auto it = csa.begin();
	if(!it) {
		database = &db;
		return true;
	}

	int storeIndex = db.typeinfo.findStore(*it, strlen(*it));
	if(storeIndex >= 0) {
		++it;
	} else {
		storeIndex = 0;
	}

	store = db.openStore(storeIndex);
	if(!store) {
		clear();
		return false;
	}

	objects[0] = *store;
	for(; it; ++it) {
		const auto& parent = objects[nesting];
		auto obj = parent.findObject(*it, strlen(*it));
		if(obj) {
			objects[++nesting] = obj;
			continue;
		}
		property = parent.findProperty(*it, strlen(*it));
		// Property must be at end of path
		if(property && !++it) {
			return true;
		}
		clear();
		return false;
	}

	return true;
}

} // namespace ConfigDB
