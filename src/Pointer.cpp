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
		const char* key = *it;
		auto keylen = strlen(key);
		String sel;

		char* brace = const_cast<char*>(strchr(key, '['));
		if(brace) {
			auto braceLen = strlen(brace);
			if(brace[braceLen - 1] != ']') {
				clear();
				return false;
			}
			sel.setString(brace + 1, braceLen - 2);
			keylen = brace - key;
		}
		auto obj = parent.findObject(key, keylen);
		if(obj) {
			objects[++nesting] = obj;
			const auto& array = objects[nesting];
			if(sel) {
				obj = array.findObject(sel.c_str(), sel.length());
				if(!obj) {
					clear();
					return false;
				}
				objects[++nesting] = obj;
			}
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
