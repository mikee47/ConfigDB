/**
 * ConfigDB/Database.h
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

#pragma once

#include "Store.h"
#include "DatabaseInfo.h"
#include <Data/CString.h>
#include <WVector.h>

namespace ConfigDB
{
class Database
{
public:
	/**
	 * @brief Database instance
	 * @param path Path to root directory where all data is stored
	 */
	Database(const DatabaseInfo& typeinfo, const String& path)
		: typeinfo(typeinfo), path(path.c_str()), updateRefs(new WeakRef[typeinfo.storeCount])
	{
	}

	virtual ~Database();

	String getName() const
	{
		auto pathstr = path.c_str();
		auto sep = strrchr(pathstr, '/');
		return sep ? &sep[1] : pathstr;
	}

	String getPath() const
	{
		return path.c_str();
	}

	/**
	 * @brief Open a store instance, load it and return a shared pointer
	 */
	StoreRef openStore(unsigned index);

	StoreUpdateRef openStoreForUpdate(unsigned index);

	/**
	 * @brief Called from StoreRef destructor so database can manage caches
	 * @param ref The store reference being destroyed
	 */
	void checkStoreRef(const StoreRef& ref);

	/**
	 * @brief Queue an asynchronous update
	 * @param store The store to update
	 * @param callback Callback which will be invoked when store is available for updates
	 */
	void queueUpdate(Store& store, Object::UpdateCallback&& callback);

	/**
	 * @brief Called by Store on completion of update so any queued updates can be started
	 * @param store The store which has just finished updating
	 * @note The next queued update (if any) is popped from the queue and scheduled for handling via the task queue.
	 */
	void checkUpdateQueue(Store& store);

	/**
	 * @brief Called from Store::commit
	 */
	bool save(Store& store) const;

	/**
	 * @brief Lock a store for writing (called by Object)
	 * @param store Store reference to be locked
	 * @retval StoreUpdateRef Invalid if called more than once
	 */
	StoreUpdateRef lockStore(StoreRef& store);

	/**
	 * @brief Get the storage format to use for a store
	 */
	virtual const Format& getFormat(const Store& store) const;

	/**
	 * @brief Called during import
	 * @param object The object to which this error relates
	 * @param arg String parameter which failed validation
	 * @param err The specific error type
	 * @retval bool Return true to continue processing, false to stop
	 *
	 * Default behaviour is to report errors but continue processing.
	 */
	virtual bool handleFormatError(FormatError err, const Object& object, const String& arg);

	/**
	 * @brief Create a read-only stream for serializing the database
	 */
	std::unique_ptr<ExportStream> createExportStream(const Format& format)
	{
		return format.createExportStream(*this);
	}

	/**
	 * @brief Serialize the database to a stream
	 */
	size_t exportToStream(const Format& format, Print& output)
	{
		return format.exportToStream(*this, output);
	}

	/**
	 * @brief Serialize the database to a single file
	 */
	bool exportToFile(const Format& format, const String& filename);

	/**
	 * @brief De-serialize the entire database from a stream
	 */
	Status importFromStream(const Format& format, Stream& source)
	{
		return format.importFromStream(*this, source);
	}

	/**
	 * @brief De-serialize the entire database from a file
	 */
	Status importFromFile(const Format& format, const String& filename);

	/**
	 * @brief Create a write-only stream for de-serializing the database
	 */
	std::unique_ptr<ImportStream> createImportStream(const Format& format)
	{
		return format.createImportStream(*this);
	}

	const DatabaseInfo& typeinfo;

private:
	/**
	 * @brief Hold a weak reference to each Store to keep track of update status
	 */
	using WeakRef = std::weak_ptr<Store>;

	/**
	 * @brief Manage a shared Store pointer to avoid un-necessary storage reading
	 */
	struct StoreCache {
		std::shared_ptr<Store> store;

		explicit operator bool() const
		{
			return bool(store);
		}

		bool operator==(const StoreCache& other) const
		{
			return store == other.store;
		}

		void reset()
		{
			store.reset();
		}

		bool typeIs(const PropertyInfo& storeInfo) const
		{
			return store && store->propinfoPtr == &storeInfo;
		}

		bool isIdle()
		{
			return store && store.use_count() == 1;
		}
	};

	/**
	 * @brief Information stored in a queue for asynchronous updates
	 */
	struct UpdateQueueItem {
		/**
		 * @brief Which store is to be updated
		 */
		uint8_t storeIndex; 

		/**
		 * @brief Application-provided callback invoked with updatable Store
		 */
		Object::UpdateCallback callback;

		bool operator==(int index) const
		{
			return int(storeIndex) == index;
		}
	};

	std::shared_ptr<Store> loadStore(const PropertyInfo& storeInfo);

	CString path;

	// Hold store open for a brief period to avoid thrashing
	static StoreCache readCache;
	static StoreCache writeCache;
	std::unique_ptr<WeakRef[]> updateRefs;
	Vector<UpdateQueueItem> updateQueue;
	bool updateQueued{false};
	static bool cacheCallbackQueued;
};

/**
 * @brief Used by code generator to create specific template for `Database`
 * @tparam ClassType Concrete type provided by code generator
 */
template <class ClassType> class DatabaseTemplate : public Database
{
public:
	DatabaseTemplate(const String& path) : Database(ClassType::typeinfo, path)
	{
	}
};

} // namespace ConfigDB
