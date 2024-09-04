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
		: typeinfo(typeinfo), path(path.c_str()), updateRefs(new UpdateRef[typeinfo.storeCount])
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
	std::shared_ptr<Store> openStore(unsigned index, bool lockForWrite = false);

	void queueUpdate(Store& store, Object::UpdateCallback callback);
	void checkUpdateQueue(Store& store);

	bool save(Store& store) const;

	/**
	 * @brief Lock a store for writing
	 * @retval bool Fails if called more than once
	 */
	bool lockStore(std::shared_ptr<Store>& store);

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

	std::unique_ptr<ExportStream> createExportStream(const Format& format)
	{
		return format.createExportStream(*this);
	}

	size_t exportToStream(const Format& format, Print& output)
	{
		return format.exportToStream(*this, output);
	}

	bool exportToFile(const Format& format, const String& filename);

	Status importFromStream(const Format& format, Stream& source)
	{
		return format.importFromStream(*this, source);
	}

	Status importFromFile(const Format& format, const String& filename);

	std::unique_ptr<ImportStream> createImportStream(const Format& format)
	{
		return format.createImportStream(*this);
	}

	const DatabaseInfo& typeinfo;

private:
	using UpdateRef = std::weak_ptr<Store>;

	struct UpdateQueueItem {
		uint8_t storeIndex;
		Object::UpdateCallback callback;

		bool operator==(int index) const
		{
			return int(storeIndex) == index;
		}
	};

	std::shared_ptr<Store> loadStore(const PropertyInfo& storeInfo);

	bool isCached(const PropertyInfo& storeInfo) const;

	CString path;

	// Hold store open for a brief period to avoid thrashing
	static std::shared_ptr<Store> cache;
	std::unique_ptr<UpdateRef[]> updateRefs;
	Vector<UpdateQueueItem> updateQueue;
	static bool callbackQueued;
};

/**
 * @brief Used by code generator to create specific template for `Database`
 * @tparam ClassType Concrete type provided by code generator
 */
template <class ClassType> class DatabaseTemplate : public Database
{
public:
	using Database::Database;
};

} // namespace ConfigDB
