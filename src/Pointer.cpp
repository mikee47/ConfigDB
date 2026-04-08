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

	objects = std::make_unique<Object[]>(csa.count());
	if(!objects) {
		return false;
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

	const Object* parent = store.get();
	for(; it; ++it) {
		const char* key = *it;
		auto keylen = strlen(key);

		auto obj = parent->findObject(key, keylen);
		if(obj) {
			parent = &objects[nesting];
			objects[nesting++] = obj;
			continue;
		}

		// Property must be at end of path
		property = parent->findProperty(key, keylen);
		if(property && !++it) {
			break;
		}
		clear();
		return false;
	}

	return true;
}

} // namespace ConfigDB
