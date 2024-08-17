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
	 * @param db Database which manages this store
	 * @param typeinfo Store type information
	 */
	Store(Database& db, const ObjectInfo& typeinfo)
		: Object(typeinfo), db(db), rootData(std::make_unique<uint8_t[]>(typeinfo.structSize))
	{
		Object::data = rootData.get();
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
		return rootData.get() + offset;
	}

	/**
	 * @brief Load store into RAM
	 *
	 * Not normally called directly by applications.
	 *
	 * Loading starts with default data loaded from schema, which is then updated during load.
	 * Failure indicates corrupt JSON file, but any readable data is available.
	 *
	 * @note All existing objects are invalidated
	 */
	virtual bool load() = 0;

	/**
	 * @brief Save store contents
	 */
	virtual bool save() = 0;

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
	void setValueString(const PropertyInfo& prop, void* data, const char* value, size_t valueLength);

	ArrayPool arrayPool;
	StringPool stringPool;

protected:
	Database& db;
	std::unique_ptr<uint8_t[]> rootData;
};

} // namespace ConfigDB
