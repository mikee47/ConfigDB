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
std::shared_ptr<Store> Database::readStoreRef;
int8_t Database::readStoreIndex{-1};
bool Database::callbackQueued;

Reader& Database::getReader(const Store&) const
{
	return Json::reader;
}

Writer& Database::getWriter(const Store&) const
{
	return Json::writer;
}

std::shared_ptr<Store> Database::openStore(unsigned index, bool lockForWrite)
{
	if(index >= typeinfo.storeCount) {
		assert(false);
		return nullptr;
	}

	if(!lockForWrite && index == unsigned(readStoreIndex)) {
		return readStoreRef;
	}

	auto& storeInfo = *typeinfo.stores[index];

	if(lockForWrite) {
		auto& updateRef = updateRefs[index];
		if(updateRef.isLocked()) {
			debug_w("[CFGDB] Store '%s' is locked, cannot write", String(storeInfo.name).c_str());
			return std::make_shared<Store>(*this);
		}

		auto store = loadStore(storeInfo);
		store->incUpdate();
		updateRef = store;
		return store;
	}

	readStoreRef.reset();
	readStoreRef = loadStore(storeInfo);
	if(!readStoreRef) {
		readStoreIndex = -1;
		return nullptr;
	}
	readStoreIndex = index;

	if(!callbackQueued) {
		System.queueCallback(InterruptCallback([]() {
			readStoreRef.reset();
			readStoreIndex = -1;
			callbackQueued = false;
		}));
		callbackQueued = true;
	}

	return readStoreRef;
}

bool Database::lockStore(std::shared_ptr<Store>& store)
{
	if(store->isLocked()) {
		return true;
	}

	auto& storeInfo = store->typeinfo();
	auto storeIndex = typeinfo.indexOf(storeInfo);
	assert(storeIndex >= 0);
	auto& updateRef = updateRefs[storeIndex];
	if(updateRef.isLocked()) {
		debug_w("[CFGDB] Store '%s' is locked, cannot write", store->getName().c_str());
		return false;
	}

	if(storeIndex == readStoreIndex) {
		readStoreRef.reset();
		readStoreIndex = -1;
	}

	if(store.use_count() == 1) {
		// No-one else is using this, just convert it to writeable
		store->incUpdate();
		return true;
	}

	// Store is in use elsewhere, so take a copy for updating
	store = std::make_unique<Store>(*store);
	store->incUpdate();
	updateRef = store;
	return true;
}

std::shared_ptr<Store> Database::loadStore(const ObjectInfo& storeInfo)
{
	debug_d("[CFGDB] LoadStore '%s'", String(storeInfo.name).c_str());

	auto store = std::make_shared<Store>(*this, storeInfo);
	if(!store) {
		return nullptr;
	}
	auto& writer = getWriter(*store);
	store->incUpdate();
	writer.loadFromFile(*store);
	store->decUpdate();
	store->dirty = false;
	return store;
}

bool Database::save(Store& store) const
{
	debug_d("[CFGDB] Save '%s'", store.getName().c_str());

	auto& reader = getReader(store);
	bool result = reader.saveToFile(store);

	// Invalidate cached stores: Some data may have changed even on failure
	int storeIndex = typeinfo.indexOf(store.typeinfo());
	if(storeIndex == readStoreIndex) {
		readStoreIndex = -1;
		readStoreRef.reset();
	}

	return result;
}

} // namespace ConfigDB
