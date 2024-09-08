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
#include "include/ConfigDB/Json/Format.h"
#include <Platform/System.h>
#include <Data/Stream/FileStream.h>
#include <Data/Buffer/PrintBuffer.h>

namespace ConfigDB
{
Database::StoreCache Database::readCache;
Database::StoreCache Database::writeCache;
bool Database::callbackQueued;

Database::~Database()
{
	if(!readCache) {
		return;
	}
	for(unsigned i = 0; i < typeinfo.storeCount; ++i) {
		if(readCache.typeIs(typeinfo.stores[i])) {
			readCache.reset();
			break;
		}
	}
}

const Format& Database::getFormat(const Store&) const
{
	return Json::format;
}

bool Database::handleFormatError(FormatError err, const Object& object, const String& arg)
{
#if DEBUG_VERBOSE_LEVEL >= WARN
	String msg;
	if(arg) {
		msg += " \"";
		msg += arg;
		msg += '"';
	}
	if(object) {
		msg += " in \"";
		msg += object.getName();
		msg += '"';
	}
	debug_e("[CFGDB] %s%s", toString(err).c_str(), msg.c_str());
#endif
	return true;
}

std::shared_ptr<Store> Database::openStore(unsigned index, bool lockForWrite)
{
	if(index >= typeinfo.storeCount) {
		assert(false);
		return nullptr;
	}

	auto& updateRef = updateRefs[index];
	auto store = updateRef.lock();

	auto& storeInfo = typeinfo.stores[index];
	if(!lockForWrite && readCache.typeIs(storeInfo) && store != readCache.store) {
		return readCache.store;
	}

	if(lockForWrite) {
		if(store) {
			if(store->isLocked()) {
				debug_w("[CFGDB] Store '%s' is locked, cannot write", String(storeInfo.name).c_str());
				return std::make_shared<Store>(*this);
			}

			store->incUpdate();
			return store;
		}

		if(readCache.typeIs(storeInfo) && readCache.store.use_count() == 1) {
			store = readCache.store;
			readCache.reset();
		} else {
			store = loadStore(storeInfo);
		}
		store->incUpdate();
		updateRef = store;
		writeCache.store = store;
		return store;
	}

	if(store && !store->isLocked()) {
		// Not locked, we can use it
		readCache.store = store;
		return store;
	}

	readCache.reset();

	store = loadStore(storeInfo);
	if(!store) {
		return nullptr;
	}

	readCache.store = store;

	if(!callbackQueued) {
		System.queueCallback(InterruptCallback([]() {
			readCache.reset();
			callbackQueued = false;
		}));
		callbackQueued = true;
	}

	return store;
}

bool Database::lockStore(std::shared_ptr<Store>& store)
{
	if(store->isLocked()) {
		return true;
	}

	auto& storeInfo = store->propinfo();
	auto storeIndex = typeinfo.indexOf(storeInfo);
	assert(storeIndex >= 0);
	auto& updateRef = updateRefs[storeIndex];
	auto storeRef = updateRef.lock();
	if(storeRef) {
		if(storeRef->isLocked()) {
			debug_w("[CFGDB] Store '%s' is locked, cannot write", store->getName().c_str());
			return false;
		}

		// Already have most recent data so update that
		store = storeRef;
		store->incUpdate();
		return true;
	}

	// If no-one else is using this store instance we can safely update it
	auto use_count = store.use_count();
	if(readCache.typeIs(storeInfo)) {
		--use_count;
		if(use_count == 1) {
			readCache.reset();
		}
	}
	if(use_count == 1) {
		updateRef = store;
		store->incUpdate();
		return true;
	}

	// Store is in use elsewhere, so take a copy for updating
	store = std::make_unique<Store>(*store);
	store->incUpdate();
	updateRef = store;
	return true;
}

std::shared_ptr<Store> Database::loadStore(const PropertyInfo& storeInfo)
{
	debug_i("[CFGDB] LoadStore '%s'", String(storeInfo.name).c_str());

	auto store = std::make_shared<Store>(*this, storeInfo);
	if(!store) {
		return nullptr;
	}

	auto& format = getFormat(*store);
	store->incUpdate();
	store->importFromFile(format);
	store->clearDirty();
	store->decUpdate();
	return store;
}

void Database::queueUpdate(Store& store, Object::UpdateCallback callback)
{
	int storeIndex = typeinfo.indexOf(store.propinfo());
	assert(storeIndex >= 0);
	updateQueue.add({uint8_t(storeIndex), callback});
}

void Database::checkUpdateQueue(Store& store)
{
	int storeIndex = typeinfo.indexOf(store.propinfo());
	int i = updateQueue.indexOf(storeIndex);
	if(i < 0) {
		return;
	}
	auto callback = std::move(updateQueue[i].callback);
	updateQueue.remove(i);
	auto storeRef = updateRefs[storeIndex].lock();
	System.queueCallback([storeRef, callback]() {
		storeRef->incUpdate();
		callback(*storeRef);
		storeRef->decUpdate();
	});
}

bool Database::save(Store& store) const
{
	debug_i("[CFGDB] Save '%s'", store.getName().c_str());

	auto& format = getFormat(store);
	bool result = store.exportToFile(format);

	// Invalidate readCache: Some data may have changed even on failure
	if(readCache.typeIs(store.propinfo())) {
		readCache.reset();
	}
	return result;
}

bool Database::exportToFile(const Format& format, const String& filename)
{
	FileStream stream;
	if(stream.open(filename, File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<512> buffer(stream);
		format.exportToStream(*this, buffer);
	}

	if(stream.getLastError() == FS_OK) {
		debug_d("[CFGDB] Database saved to '%s'", filename.c_str());
		return true;
	}

	debug_e("[CFGDB] Database save to '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
	return false;
}

Status Database::importFromFile(const Format& format, const String& filename)
{
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		debug_w("[CFGDB] open '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
		return Status::fileError(stream.getLastError());
	}

	return format.importFromStream(*this, stream);
}

} // namespace ConfigDB
