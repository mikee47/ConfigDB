/**
 * ConfigDB/Array.cpp
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
#include "include/ConfigDB/Json/Store.h"
#include <Platform/System.h>

namespace ConfigDB
{
const ObjectInfo* Database::storeType;
std::shared_ptr<Store> Database::storeRef;
bool Database::callbackQueued;

Store* Database::createStore(const ObjectInfo& typeinfo)
{
	return new Json::Store(*this, typeinfo);
}

std::shared_ptr<Store> Database::openStore(const ObjectInfo& typeinfo)
{
	if(storeType == &typeinfo) {
		return storeRef;
	}

	storeRef.reset();

	auto store = createStore(typeinfo);
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

	inst->load();
	return inst;
}

} // namespace ConfigDB
