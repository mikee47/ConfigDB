/**
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

#include "include/ConfigDB/Database.h"
#include "include/ConfigDB/Json/Reader.h"
#include "include/ConfigDB/Json/Writer.h"
#include <Platform/System.h>

namespace ConfigDB
{
const ObjectInfo* Database::storeType;
std::shared_ptr<Store> Database::storeRef;
bool Database::callbackQueued;

Reader& Database::getReader(const Store& store) const
{
	return Json::reader;
}

Writer& Database::getWriter(const Store& store) const
{
	return Json::writer;
}

std::shared_ptr<Store> Database::openStore(const ObjectInfo& typeinfo)
{
	if(storeType == &typeinfo) {
		return storeRef;
	}

	storeRef.reset();

	auto store = new Store(*this, typeinfo);
	if(!store) {
		storeType = nullptr;
		return nullptr;
	}

	if(!callbackQueued) {
		System.queueCallback(InterruptCallback([]() {
			storeRef.reset();
			storeType = nullptr;
			callbackQueued = false;
		}));
		callbackQueued = true;
	}

	std::shared_ptr<Store> inst(store);
	storeRef = inst;
	storeType = &typeinfo;

	auto& writer = getWriter(*inst);
	writer.loadFromFile(*inst);

	return inst;
}

std::shared_ptr<Store> Database::findStore(const char* name, size_t nameLength)
{
	int i = typeinfo.findStore(name, nameLength);
	return i >= 0 ? openStore(*typeinfo.stores[i]) : nullptr;
}

bool Database::save(Store& store) const
{
	auto& reader = getReader(store);
	return reader.saveToFile(store);
}

} // namespace ConfigDB
