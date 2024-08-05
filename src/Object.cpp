/**
 * ConfigDB/Object.cpp
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

#include "include/ConfigDB/Object.h"
#include "include/ConfigDB/Store.h"

namespace ConfigDB
{
bool Object::commit()
{
	return getStore().commit();
}

Database& Object::getDatabase()
{
	return getStore().getDatabase();
}

String Object::getPath() const
{
	String path;
	if(parent) {
		path += parent->getPath();
	} else {
		path += getStore().getName();
	}
	path += '.';
	path += getName();
	return path;
}

} // namespace ConfigDB
