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

#include <WString.h>
#include <Data/CString.h>

namespace ConfigDB
{
class Store;

/**
 * @brief Serialisation format
 */
enum class Format {
	Compact,
	Pretty,
};

class Database
{
public:
	/**
	 * @brief Database instance
	 * @param path Path to root directory where all data is stored
	 */
	Database(const String& path) : path(path.c_str())
	{
	}

	virtual ~Database()
	{
	}

	/**
	 * @brief Set number of spaces to indent when serialising output
	 * @param indent 0 produces compact output, > 0 lays content out for easier reading
	 */
	void setFormat(Format format)
	{
		this->format = format;
	}

	Format getFormat() const
	{
		return format;
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
	 * @brief Get stores
	 */
	virtual std::shared_ptr<Store> getStore(unsigned index) = 0;

	/**
	 * @brief Keep a lock on the most recently opened store
	 *
	 * Called by `StoreTemplate::open`.
	 *
	 * Improves performance but also absolutely necessary due to the way object instances
	 * are constructed. For example, here 
	 *
	 *   class GeneralStore: public StoreTemplate...
	 *
     *   class General: public ContainedGeneral
     *   {
     *   public:
     *       General(BasicConfig& db):
     *           ContainedGeneral(*GeneralStore::open(db), fstr_general),
	 * 			 store(GeneralStore::open(db))
     *       {
     *       }
	 *
     *       std::shared_ptr<GeneralStore> store;
     *   };
	 *
	 * The call to `GeneralStore::open(db)` creates a Store instance, calls this method
	 * then returns it for use by the `ContainedGeneral` base class constructor.
	 * Because we are holding a lock on the pointer it remains in scope and so
	 * our own `store` variable gets set correctly.
	 *
	 * The lock is held here to avoid unintentionally holding more than one store open
	 * at a time.
	 */
	void setActiveStore(std::shared_ptr<Store> store)
	{
		// TODO: Implement timeout mechanism for releasing this
		activeStore = store;
	}

private:
	CString path;
	std::shared_ptr<Store> activeStore;
	Format format{};
};

} // namespace ConfigDB
