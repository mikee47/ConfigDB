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

/**
 * @brief Manages access to an object store, typically one file
 */
class Store : public Object
{
public:
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
	}

	Store(const Store&) = delete;

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
		return writeCheck() ? rootData.get() : nullptr;
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

	bool isReadOnly() const
	{
		return readOnly;
	}

	bool writeCheck() const;

protected:
	friend class Object;
	friend class ArrayBase;
	friend class Database;

	ArrayPool arrayPool;
	StringPool stringPool;

private:
	Database& db;
	std::unique_ptr<uint8_t[]> rootData;
	bool readOnly{};
};

} // namespace ConfigDB
