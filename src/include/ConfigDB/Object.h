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

#include "Store.h"
#include "Property.h"

namespace ConfigDB
{
/**
 * @brief An object can contain other objects, properties and arrays
 */
class Object
{
public:
	Object(const String& name) : name(name)
	{
	}

	virtual ~Object()
	{
	}

	const String& getName() const
	{
		return name;
	}

	String getPath() const
	{
		String path = getStore()->getName();
		if(path && name) {
			path += '.';
			path += name;
		}
		return path;
	}

	virtual String getStringValue(const String& key) const = 0;

	virtual std::shared_ptr<Store> getStore() const = 0;

	/**
	 * @brief Get child objects
	 */
	virtual std::unique_ptr<Object> getObject(unsigned index) const = 0;

	/**
	 * @brief Get properties
	 */
	virtual Property getProperty(unsigned index) = 0;

	/**
	 * @brief Commit changes to the store
	 */
	bool commit()
	{
		return getStore()->commit();
	}

	virtual size_t printTo(Print& p) const = 0;

private:
	String name;
};

} // namespace ConfigDB
