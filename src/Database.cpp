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
bool Database::cacheCallbackQueued;

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

StoreRef Database::openStore(unsigned index)
{
	if(index >= typeinfo.storeCount) {
		assert(false);
		return std::make_shared<Store>(*this);
	}

	auto& storeInfo = typeinfo.stores[index];

	// Write cache contains the most recent store data, can use it provided it's been committed
	if(writeCache.typeIs(storeInfo) && !writeCache.store->isDirty()) {
		if(writeCache.store->isLocked()) {
			// Store has active updaters, take a copy
			readCache.reset();
			readCache.store = std::make_shared<Store>(*writeCache.store);
		} else {
			// No updaters, use store directly
			readCache.store = writeCache.store;
			writeCache.reset();
		}
		return readCache.store;
	}

	// Second preference is read cache
	if(readCache.typeIs(storeInfo)) {
		assert(!readCache.store->isLocked());
		return readCache.store;
	}

	// Need to load from storage
	readCache.reset();
	readCache.store = loadStore(storeInfo);

	return readCache.store;
}

StoreUpdateRef Database::openStoreForUpdate(unsigned index)
{
	auto store = openStore(index);
	return lockStore(store);
}

StoreUpdateRef Database::lockStore(StoreRef& store)
{
	CFGDB_DEBUG("");

	auto invalid = [this]() -> StoreUpdateRef {
		StoreRef invalid = std::make_shared<Store>(*this);
		return invalid;
	};

	assert(store);
	if(!store) {
		return invalid();
	}

	if(store->isLocked()) {
		writeCache.store = store;
		return store;
	}

	auto& storeInfo = store->propinfo();
	auto storeIndex = typeinfo.indexOf(storeInfo);
	assert(storeIndex >= 0);
	auto& weakRef = updateRefs[storeIndex];

	if(auto ref = weakRef.lock()) {
		if(ref->isLocked()) {
			debug_w("[CFGDB] Store '%s' is locked, cannot write", store->getName().c_str());
			return invalid();
		}
	}

	if(writeCache.typeIs(storeInfo)) {
		assert(readCache.store != writeCache.store);
		store = writeCache.store;
		weakRef = store;
		return store;
	}

	//  If noone else is using the provided Store instance, we can update it directly
	int use_count = store.use_count();
	if(readCache.store == store) {
		--use_count;
	}
	if(use_count <= 1) {
		weakRef = store;
		writeCache.store = store;
		return store;
	}

	// Before allocating more memory, see if we can dispose of some
	if(readCache.isIdle()) {
		readCache.reset();
	}

	// Store is in use elsewhere, so load a copy from storage
	store = std::make_shared<Store>(*store);
	weakRef = store;
	writeCache.store = store;
	return store;
}

std::shared_ptr<Store> Database::loadStore(const PropertyInfo& storeInfo)
{
	debug_i("[CFGDB] LoadStore '%s'", String(storeInfo.name).c_str());

	StoreRef store = std::make_shared<Store>(*this, storeInfo);
	if(!store) {
		return nullptr;
	}

	auto& format = getFormat(*store);
	StoreUpdateRef update = store;
	update->importFromFile(format);
	update->clearDirty();
	return store;
}

void Database::queueUpdate(Store& store, Object::UpdateCallback&& callback)
{
	int storeIndex = typeinfo.indexOf(store.propinfo());
	assert(storeIndex >= 0);
	updateQueue.add({uint8_t(storeIndex), std::move(callback)});
}

void Database::checkStoreRef(const StoreRef& ref)
{
	bool isCached = false;
	auto useCount = ref.use_count();
	if(readCache.store == ref) {
		isCached = true;
		--useCount;
	}
	if(writeCache.store == ref) {
		isCached = true;
		--useCount;
	}

	if(!isCached || useCount != 1) {
		return;
	}

	// Schedule cache to be cleared

	if(cacheCallbackQueued) {
		return;
	}

	System.queueCallback(
		[](void* param) {
			auto db = static_cast<Database*>(param);

			cacheCallbackQueued = false;

			if(db->updateQueued || !db->updateQueue.isEmpty()) {
				return;
			}

			if(readCache.isIdle()) {
				readCache.reset();
			}
			if(writeCache.isIdle()) {
				writeCache.reset();
			}
		},
		this);
	cacheCallbackQueued = true;
}

void Database::checkUpdateQueue(Store& store)
{
	/*
	We get called when an updater is done, even if nothing was saved.
	This is therefore a good location to retire stale read cache.
	*/
	auto& storeInfo = store.propinfo();
	if(readCache.typeIs(storeInfo)) {
		readCache.reset();
	}

	int storeIndex = typeinfo.indexOf(storeInfo);
	int i = updateQueue.indexOf(storeIndex);
	if(i < 0) {
		return;
	}
	auto callback = std::move(updateQueue[i].callback);
	updateQueue.remove(i);
	System.queueCallback([this, storeIndex, callback]() {
		updateQueued = false;
		auto store = openStoreForUpdate(storeIndex);
		assert(store);
		callback(*store);
	});
	updateQueued = true;
}

bool Database::save(Store& store) const
{
	debug_i("[CFGDB] Save '%s'", store.getName().c_str());

	auto& format = getFormat(store);
	bool result = store.exportToFile(format);

	// Update write cache so it contains most recent data
	auto storeIndex = typeinfo.indexOf(store.propinfo());
	assert(storeIndex >= 0);
	writeCache.store = updateRefs[storeIndex].lock();
	assert(&store == writeCache.store.get());

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
