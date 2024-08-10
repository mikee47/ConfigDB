/**
 * ConfigDB/Json/Object.cpp
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

#include <ConfigDB/Json/Object.h>
#include <ConfigDB/Json/ObjectArray.h>
#include <ConfigDB/Json/Store.h>
#include <ConfigDB/Database.h>

namespace ConfigDB::Json
{
// Object::Object(ObjectArray& parent, unsigned index) : ConfigDB::Object(parent) //, object(parent.array[index])
// {
// }

// Object::Object(ObjectArray& parent) : ConfigDB::Object(parent) //, object(parent.array.createNestedObject())
// {
// }

size_t Object::printTo(Print& p) const
{
	return Store::printObjectTo(object, getDatabase().getFormat(), p);
}

} // namespace ConfigDB::Json
