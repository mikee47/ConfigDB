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
#include <WString.h>
#include <Data/CString.h>

namespace ConfigDB
{
struct DatabaseInfo {
	volatile uint32_t storeCount : 8;
	const StoreInfo* stores[];
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

	virtual const DatabaseInfo& getTypeinfo() const = 0;

private:
	CString path;
	Format format{};
};

/**
 * @brief Used by code generator to create specific template for `Database`
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 */
template <class ClassType> class DatabaseTemplate : public Database
{
public:
	using Database::Database;

	const DatabaseInfo& getTypeinfo() const override
	{
		return static_cast<const ClassType*>(this)->typeinfo;
	}
};

} // namespace ConfigDB
