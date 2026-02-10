/****
 * ConfigDB/Database.cpp
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

#include "include/ConfigDB/Pointer.h"
#include <Data/CStringArray.h>

namespace ConfigDB
{
PointerContext Pointer::resolve(Database& db)
{
	CStringArray csa;
	{
		String tmp = string;
		if(tmp[0] == '/') {
			tmp.remove(0, 1);
		}
		tmp.replace('/', '\0');
		csa = std::move(tmp);
	}
	auto it = csa.begin();
	if(!it) {
		return PointerContext(db);
	}

	int storeIndex = db.typeinfo.findStore(*it, strlen(*it));
	if(storeIndex >= 0) {
		++it;
	} else {
		storeIndex = 0;
	}

	unsigned offset{0};
	auto prop = &db.typeinfo.stores[storeIndex];
	for(; it; ++it) {
		int i = prop->findObject(*it, strlen(*it));
		if(i < 0) {
			return PointerContext();
		}
		offset += prop->offset;
		prop = &prop->getObject(i);
	}

	auto store = db.openStore(storeIndex);
	if(!store) {
		return PointerContext();
	}

	Object obj(*store, *prop, offset);
	return PointerContext(store, obj);
}

} // namespace ConfigDB
