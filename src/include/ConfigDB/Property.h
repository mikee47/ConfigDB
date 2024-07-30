/**
 * ConfigDB/Property.h
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
#include <memory>

#define CONFIGDB_PROPERTY_TYPE_MAP(XX)                                                                                 \
	XX(String)                                                                                                         \
	XX(Integer)                                                                                                        \
	XX(Boolean)

namespace ConfigDB
{
class Object;

/**
 * @brief Manages a key/value pair
 */
class Property
{
public:
	enum class Type {
#define XX(name) name,
		CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
	};

	Property(Object& object) : object(object)
	{
	}

	Property(Object& object, const FlashString& name, Type type, const FlashString* defaultValue)
		: object(object), name(&name), defaultValue(defaultValue), type(type)
	{
	}

	String getName() const
	{
		if(name) {
			return *name;
		}
		return nullptr;
	}

	String getDefaultStringValue() const
	{
		if(defaultValue) {
			return *defaultValue;
		}
		return nullptr;
	}

	String getStringValue() const;

	Type getType() const
	{
		return type;
	}

	explicit operator bool() const
	{
		return bool(name);
	}

	String getJsonValue() const;

private:
	Object& object;
	const FlashString* name{};
	const FlashString* defaultValue{};
	Type type{};
};

} // namespace ConfigDB

String toString(ConfigDB::Property::Type type);
