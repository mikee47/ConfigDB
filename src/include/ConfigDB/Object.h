/**
 * ConfigDB/Object.h
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

#include "Property.h"
#include <Printable.h>

namespace ConfigDB
{
class Database;
class Store;

/**
 * @brief An object can contain other objects, properties and arrays
 */
class Object : public Printable
{
public:
	Object() = default;

	explicit Object(Object& parent) : parent(&parent)
	{
	}

	/**
	 * @brief Retrieve a value
	 * @param key Key for value
	 * @retval String
	 */
	virtual String getStringValue(const String& key) const = 0;

	/**
	 * @brief Retrieve a value by index
	 * @param key Index of value
	 * @retval String
	 */
	virtual String getStringValue(unsigned index) const = 0;

	/**
	 * @brief Store a value
	 * @param key Key for value
	 * @param value Value to store
	 * @retval bool true on success
	 */
	// virtual bool setStringValue(const String& key, const String& value) = 0;

	virtual Store& getStore()
	{
		assert(parent != nullptr);
		return parent->getStore();
	}

	Database& getDatabase();

	const Database& getDatabase() const
	{
		return const_cast<Object*>(this)->getDatabase();
	}

	/**
	 * @brief Get number of child objects
	 */
	virtual unsigned getObjectCount() const = 0;

	/**
	 * @brief Get child object by key
	 */
	virtual std::unique_ptr<Object> getObject(const String& key) = 0;

	/**
	 * @brief Get child object by index
	 */
	virtual std::unique_ptr<Object> getObject(unsigned index) = 0;

	/**
	 * @brief Get number of properties
	 */
	virtual unsigned getPropertyCount() const = 0;

	/**
	 * @brief Get properties
	 */
	virtual Property getProperty(unsigned index) = 0;

	/**
	 * @brief Commit changes to the store
	 */
	bool commit();

	// Printable [STOREIMPL]
	// virtual size_t printTo(Print& p) const = 0;

protected:
	Object* parent{};
};

} // namespace ConfigDB
