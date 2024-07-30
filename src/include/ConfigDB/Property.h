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
		Invalid,
		Null,
		String,
		Integer,
		Boolean,
	};

	Property(Object& object) : object(object)
	{
	}

	Property(Object& object, const String& name, Type type) : object(object), name(name), type(type)
	{
	}

	const String& getName() const
	{
		return name;
	}

	String getStringValue() const;

	Type getType() const
	{
		return type;
	}

	explicit operator bool() const
	{
		return type != Type::Invalid;
	}

	String getJsonValue() const;

private:
	Object& object;
	String name;
	Type type{};
};

} // namespace ConfigDB
