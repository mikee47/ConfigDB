/**
 * ConfigDB/Store.h
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

#include "Database.h"
#include "Object.h"
#include <debug_progmem.h>

namespace ConfigDB
{
/**
 * @brief Manages access to an object store, typically one file
 */
class Store : public Object
{
public:
	/**
	 * @brief Storage instance
	 * @param db Database to which this store belongs
	 * @param name Name of store, used as key in JSONPath
	 */
	Store(Database& db, const String& name) : Object(), db(db), name(name)
	{
		debug_d("%s(%s)", __FUNCTION__, name.c_str());
	}

	virtual ~Store()
	{
		debug_d("%s(%s)", __FUNCTION__, name.c_str());
	}

	virtual bool commit() = 0;

	const String& getName() const
	{
		return name;
	}

	String getPath() const
	{
		String path = db.getPath();
		path += '/';
		path += name ?: F("_root");
		return path;
	}

	Store& getStore() override
	{
		return *this;
	}

	Database& getDatabase() const override
	{
		return db;
	}

private:
	Database& db;
	String name;
};

template <class BaseType, class ClassType> class StoreTemplate : public BaseType
{
public:
	using BaseType::BaseType;

	static std::shared_ptr<ClassType> open(Database& db)
	{
		auto inst = store.lock();
		if(!inst) {
			inst = std::make_shared<ClassType>(db);
			store = inst;
		}
		return inst;
	}

	static std::shared_ptr<ClassType> getPointer()
	{
		return store.lock();
	}

private:
	static std::weak_ptr<ClassType> store;
};

template <class BaseType, class ClassType> std::weak_ptr<ClassType> StoreTemplate<BaseType, ClassType>::store;
} // namespace ConfigDB
