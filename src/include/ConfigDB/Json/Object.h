/**
 * ConfigDB/Json/Object.h
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
#include "../Object.h"

namespace ConfigDB::Json
{
/**
 * @brief Access a Object of simple key/value pairs within a store
 */
class Object : public ConfigDB::Object
{
public:
	Object(std::shared_ptr<Store> store, const String& path) : ConfigDB::Object(path), store(store)
	{
	}

	template <typename T> bool setValue(const String& key, const T& value)
	{
		return store->setValue(getName(), key, value);
	}

	template <typename T> T getValue(const String& key)
	{
		return store->getValue<T>(getName(), key);
	}

	std::shared_ptr<ConfigDB::Store> getStore() const override
	{
		return store;
	}

	size_t printTo(Print& p) const override;

protected:
	std::shared_ptr<Store> store;
};

} // namespace ConfigDB::Json
