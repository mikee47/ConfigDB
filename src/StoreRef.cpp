/**
 * ConfigDB/StoreRef.cpp
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

#include "include/ConfigDB/StoreRef.h"
#include "include/ConfigDB/Store.h"

namespace ConfigDB
{
StoreRef::~StoreRef()
{
	if(*this) {
		get()->checkRef(*this);
	}
}

StoreUpdateRef::~StoreUpdateRef()
{
	if(*this) {
		get()->decUpdate();
	}
}

StoreUpdateRef& StoreUpdateRef::operator=(StoreRef& other)
{
	if(*this) {
		get()->decUpdate();
	}
	StoreRef::operator=(other);
	if(*this) {
		get()->incUpdate();
	}
	return *this;
}

} // namespace ConfigDB
