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

std::shared_ptr<Store> Database::openStore(unsigned index, bool forWrite)
{
	if(index >= typeinfo.storeCount) {
		assert(false);
		return nullptr;
	}

	if(!forWrite && index == unsigned(readStoreIndex)) {
		return readStoreRef;
	}

	auto& storeInfo = *typeinfo.stores[index];

	if(forWrite) {
		auto& lock = locks[index];
		if(lock) {
			debug_w("[CFGDB] Store '%s' is locked, cannot write", String(storeInfo.name).c_str());
			return std::make_shared<Store>(*this);
		}

		lock = std::make_shared<Store>(*this, storeInfo);
		return lock.ref;
	}

	readStoreRef.reset();
	readStoreRef = std::make_shared<Store>(*this, storeInfo);
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

	auto& writer = getWriter(*readStoreRef);
	writer.loadFromFile(*readStoreRef);

	readStoreRef->readOnly = true;

	return readStoreRef;
}

bool Database::unlock(std::shared_ptr<Store>& store)
{
	if(!store->isReadOnly()) {
		return true;
	}

	auto storeIndex = typeinfo.indexOf(store->typeinfo());
	assert(storeIndex >= 0);
	auto& lock = locks[storeIndex];
	if(lock) {
		debug_e("[CFGDB] Store locked for writing");
		return false;
	}

	if(storeIndex == readStoreIndex) {
		readStoreRef.reset();
		readStoreIndex = -1;
	}

	lock = store;
	store->readOnly = false;
	return true;
}

bool Database::save(Store& store) const
{
	auto& reader = getReader(store);
	bool result = reader.saveToFile(store);

	// Invalidate cached stores: Some data may have changed even on failure
	int storeIndex = typeinfo.indexOf(store.typeinfo());
	if(storeIndex == readStoreIndex) {
		readStoreIndex = -1;
		readStoreRef = nullptr;
	}

	return result;
}

} // namespace ConfigDB
