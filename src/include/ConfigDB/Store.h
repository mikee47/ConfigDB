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
#include <debug_progmem.h>

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

struct StoreInfo {
	// DO NOT access these directly!
	const FlashString* name; ///< Root store always nullptr
	const ObjectInfo& object;

	static const StoreInfo& empty()
	{
		static const StoreInfo PROGMEM emptyInfo{.object = ObjectInfo::empty()};
		return emptyInfo;
	}

	String getName() const
	{
		return name ? String(*name) : nullptr;
	}

	/**
	 * @brief Root object in a store has no name
	 */
	bool isRoot() const
	{
		return name == nullptr;
	}

	bool operator==(const String& s) const
	{
		return name ? *name == s : s.length() == 0;
	}
};

/**
 * @brief Manages access to an object store, typically one file
 */
class Store : public Printable
{
public:
	/**
	 * @brief Storage instance
	 * @param db Database to which this store belongs
	 * @param name Name of store, used as key in JSONPath
	 */
	Store(Database& db, const String& name) : db(db), name(name)
	{
		debug_d("%s(%s)", __FUNCTION__, name.c_str());
	}

	~Store()
	{
		debug_d("%s(%s)", __FUNCTION__, name.c_str());
	}

	const String& getName() const
	{
		return name;
	}

	String getPath() const;

	Database& getDatabase() const
	{
		return db;
	}

	void* getObjectDataPtr(const ObjectInfo& object);

	void* getObjectArrayDataPtr(ArrayId arrayId, unsigned index);

	template <typename T> T& getObjectData(const ObjectInfo& object)
	{
		return *reinterpret_cast<T*>(getObjectDataPtr(object));
	}

	virtual bool commit() = 0;

	virtual std::unique_ptr<Object> getObject() = 0;

	virtual const StoreInfo& getTypeinfo() const = 0;

	ObjectPool objectPool;
	ArrayPool arrayPool;
	ObjectArrayPool objectArrayPool;
	StringPool stringPool;

protected:
	void clear()
	{
		arrayPool.clear();
		objectArrayPool.clear();
		objectPool.clear();
		stringPool.clear();
	}

	Database& db;
	String name;
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
		}
		return inst;
	}

	const StoreInfo& getTypeinfo() const override
	{
		return static_cast<const ClassType*>(this)->typeinfo;
	}

protected:
	static std::weak_ptr<ClassType> store;
};

template <class BaseType, class ClassType> std::weak_ptr<ClassType> StoreTemplate<BaseType, ClassType>::store;

} // namespace ConfigDB
