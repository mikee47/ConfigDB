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
#include "Reader.h"
#include "Writer.h"
#include "DatabaseInfo.h"
#include <Data/CString.h>

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
		: typeinfo(typeinfo), path(path.c_str()), locks(new WriterLock[typeinfo.storeCount])
	{
	}

	virtual ~Database()
	{
	}

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
	std::shared_ptr<Store> openStore(unsigned index, bool forWrite = false);

	std::shared_ptr<Store> openStore(const ObjectInfo& objinfo, bool forWrite = false)
	{
		return openStore(typeinfo.indexOf(objinfo), forWrite);
	}

	bool save(Store& store) const;

	/**
	 * @brief Unlock a store for writing
	 * @retval bool Fails if called more than once
	 */
	bool unlock(std::shared_ptr<Store>& store);

	virtual Reader& getReader(const Store& store) const;
	virtual Writer& getWriter(const Store& store) const;

	const DatabaseInfo& typeinfo;

private:
	struct WriterLock {
		std::shared_ptr<Store> ref;
		std::weak_ptr<Store> weakref;

		explicit operator bool() const
		{
			return weakref.use_count() > 1;
		}

		WriterLock& operator=(std::shared_ptr<Store> ref)
		{
			this->ref = ref;
			this->weakref = ref;
			return *this;
		}
	};

	CString path;

	// Hold store open for a brief period to avoid thrashing
	static std::shared_ptr<Store> readStoreRef;
	std::unique_ptr<WriterLock[]> locks;
	static int8_t readStoreIndex;
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
