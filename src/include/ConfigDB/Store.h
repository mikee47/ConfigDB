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

#include "Object.h"
#include "Array.h"
#include "ObjectArray.h"
#include "Pool.h"
#include <WString.h>
#include <memory>

namespace ConfigDB
{
class Database;
class Object;
class ObjectInfo;

/**
 * @brief Serialisation format
 */
enum class Format {
	Compact,
	Pretty,
};

/**
 * @brief Manages access to an object store, typically one file
 */
class Store : public Object, public Printable
{
public:
	/**
	 * @brief Storage instance
	 * @param db Database to which this store belongs
	 * @param name Name of store, used as key in JSONPath
	 */
	Store(Database& db, const ObjectInfo& typeinfo) : Object(typeinfo), db(db)
	{
	}

	String getFileName() const
	{
		String name = getName();
		return name.length() ? name : F("_root");
	}

	String getFilePath() const;

	Database& getDatabase() const
	{
		return db;
	}

	void* getObjectDataPtr(const ObjectInfo& object)
	{
		size_t offset{0};
		for(auto obj = &object; obj; obj = obj->parent) {
			offset += obj->getOffset();
		}
		return rootObjectData.get() + offset;
	}

	virtual bool commit() = 0;

	/**
	 * @brief Print object
	 * @param object Object to print
	 * @param name Name to print with object. If nullptr, omit opening/closing braces.
	 * @param nesting Nesting level for pretty-printing
	 * @param p Output stream
	 * @retval size_t Number of characters written
	 */
	virtual size_t printObjectTo(const Object& object, const FlashString* name, unsigned nesting, Print& p) const = 0;

	size_t printTo(Print& p, unsigned nesting) const;

	size_t printTo(Print& p) const override
	{
		return printTo(p, 0);
	}

	String getValueString(const PropertyInfo& info, const void* data) const;
	bool setValueString(const PropertyInfo& prop, void* data, const char* value, size_t valueLength);

	std::unique_ptr<uint8_t[]> rootObjectData;
	ArrayPool arrayPool;
	StringPool stringPool;

protected:
	void clear()
	{
		arrayPool.clear();
		rootObjectData.reset();
		stringPool.clear();
	}

	Database& db;
};

/**
 * @brief Used by store implemention to create specific template for `Store`
 * @tparam BaseType The store's base `Store` class
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 */
template <class BaseType, class ClassType> class StoreTemplate : public BaseType
{
public:
	using BaseType::BaseType;

	/**
	 * @brief Open a store instance, load it and return a shared pointer
	 */
	static std::shared_ptr<ClassType> open(Database& db)
	{
		auto inst = store.lock();
		if(!inst) {
			inst = std::make_shared<ClassType>(db);
			store = inst;
			inst->load();
		}
		return inst;
	}

protected:
	static std::weak_ptr<ClassType> store;
};

template <class BaseType, class ClassType> std::weak_ptr<ClassType> StoreTemplate<BaseType, ClassType>::store;

} // namespace ConfigDB
