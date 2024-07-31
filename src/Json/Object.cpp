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
#include <Data/CStringArray.h>
#include <Data/Stream/MemoryDataStream.h>

namespace ConfigDB::Json
{
size_t Object::printTo(Print& p) const
{
	auto& db = getStore()->database();
	auto obj = store->getJsonObject(getName());
	switch(db.getFormat()) {
	case Format::Compact:
		return serializeJson(obj, p);
	case Format::Pretty:
		return serializeJsonPretty(obj, p);
	}
	return 0;
}

} // namespace ConfigDB::Json
