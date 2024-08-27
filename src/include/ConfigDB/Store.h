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

#if DEBUG_VERBOSE_LEVEL == DBG
#include <debug_progmem.h>
#define CFGDB_DEBUG(fmt, ...) debug_e("[CFGDB] %s() %s %p" fmt, __FUNCTION__, getName().c_str(), this, ##__VA_ARGS__);
#else
#define CFGDB_DEBUG(...)
#endif

namespace ConfigDB
{
class Database;

/**
 * @brief Manages access to an object store, typically one file
 */
class Store : public Object
{
public:
	/**
	 * @brief This is an empty Store instance
	 * Object chain is built via references so need a valid Store instance which can be identifyed as empty.
	 * We don't update the instance count here since it will never contain anything
	 */
	Store(Database& db) : Object(), db(db)
	{
	}

	/**
	 * @brief Storage instance
	 * @param db Database which manages this store
	 * @param typeinfo Store type information
	 */
	Store(Database& db, const ObjectInfo& typeinfo)
		: Object(typeinfo), db(db), rootData(std::make_unique<uint8_t[]>(typeinfo.structSize))
	{
		++instanceCount;
		CFGDB_DEBUG(" %u", instanceCount)
	}

	/**
	 * @brief Copy constructor
	 */
	explicit Store(const Store& store)
		: Object(store.typeinfo()), arrayPool(store.arrayPool), stringPool(store.stringPool), db(store.db),
		  rootData(std::make_unique<uint8_t[]>(store.typeinfo().structSize))
	{
		++instanceCount;
		CFGDB_DEBUG(" COPY %u", instanceCount)
		memcpy(rootData.get(), store.rootData.get(), typeinfo().structSize);
	}

	Store(Store&&) = delete;

	~Store()
	{
		if(*this) {
			--instanceCount;
			CFGDB_DEBUG(" %u", instanceCount);
		}
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

	uint16_t getObjectDataRef(const ObjectInfo& object)
	{
		size_t offset{0};
		for(auto obj = &object; obj; obj = obj->parent) {
			offset += obj->getOffset();
		}
		return offset;
	}

	uint8_t* getRootData()
	{
		if(!writeCheck()) {
			return nullptr;
		}
		dirty = true;
		return rootData.get();
	}

	const uint8_t* getRootData() const
	{
		return rootData.get();
	}

	/**
	 * @brief Reset store contents to defaults
	 * @note Use caution! All reference objects will be invalidated by this call
	 */
	void clear();

	String getValueString(const PropertyInfo& info, const void* data) const;
	PropertyData parseString(const PropertyInfo& prop, const char* value, uint16_t valueLength);

	const StringPool& getStringPool() const
	{
		return stringPool;
	}

	const ArrayPool& getArrayPool() const
	{
		return arrayPool;
	}

	bool isLocked() const
	{
		return updaterCount != 0;
	}

	/**
	 * @brief Get number of valid (non-empty) Store instances in existence
	 */
	static uint8_t getInstanceCount()
	{
		return instanceCount;
	}

	bool writeCheck() const;

	using Object::exportToFile;

	bool exportToFile(const Format& format) const
	{
		String filename = getFilePath() + format.getFileExtension();
		return exportToFile(format, filename);
	}

	bool importFromFile(const Format& format, const String& filename)
	{
		incUpdate();
		bool result = Object::importFromFile(format, filename);
		decUpdate();
		return result;
	}

	bool importFromFile(const Format& format)
	{
		String filename = getFilePath() + format.getFileExtension();
		return importFromFile(format, filename);
	}

protected:
	friend class Object;
	friend class ArrayBase;
	friend class Database;

	void queueUpdate(Object::UpdateCallback callback);

	bool commit();

	void clearDirty()
	{
		dirty = false;
	}

	void incUpdate()
	{
		++updaterCount;
		CFGDB_DEBUG(" %u", updaterCount)
	}

	void decUpdate();

	ArrayPool arrayPool;
	StringPool stringPool;

private:
	Database& db;
	std::unique_ptr<uint8_t[]> rootData;
	uint8_t updaterCount{};
	bool dirty{};
	static uint8_t instanceCount;
};

} // namespace ConfigDB
