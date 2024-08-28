/**
 * ConfigDB/Property.cpp
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

#include "include/ConfigDB/Database.h"
#include <debug_progmem.h>

namespace ConfigDB
{
uint8_t Store::instanceCount;

void Store::clear()
{
	auto& root = typeinfo();
	memcpy_P(rootData.get(), root.defaultData, root.structSize);
	stringPool.clear();
	arrayPool.clear();
	dirty = true;
}

String Store::getFilePath() const
{
	String path = db.getPath();
	path += '/';
	path += getFileName();
	return path;
}

String Store::getValueString(const PropertyInfo& info, const void* data) const
{
	auto& propData = *static_cast<const PropertyData*>(data);
	switch(info.type) {
	case PropertyType::Boolean:
		return propData.b ? "true" : "false";
	case PropertyType::Int8:
		return String(propData.int8);
	case PropertyType::Int16:
		return String(propData.int16);
	case PropertyType::Int32:
		return String(propData.int32);
	case PropertyType::Int64:
		return String(propData.int64);
	case PropertyType::UInt8:
		return String(propData.uint8);
	case PropertyType::UInt16:
		return String(propData.uint16);
	case PropertyType::UInt32:
		return String(propData.uint32);
	case PropertyType::UInt64:
		return String(propData.uint64);
	case PropertyType::String:
		if(propData.string) {
			return String(stringPool[propData.string]);
		}
		if(info.defaultValue.string) {
			return *info.defaultValue.string;
		}
		return nullptr;
	}
	return nullptr;
}

PropertyData Store::parseString(const PropertyInfo& prop, const char* value, uint16_t valueLength)
{
	if(prop.type == PropertyType::String) {
		if(!value || (prop.defaultValue.string && *prop.defaultValue.string == value)) {
			return {.string = 0};
		}
		return {.string = stringPool.findOrAdd({value, valueLength})};
	}

	if(!value) {
		PropertyData dst{};
		dst.setValue(prop, nullptr);
		return dst;
	}

	PropertyData src{};

	switch(prop.type) {
	case PropertyType::Boolean:
		return {.b = (valueLength == 4) && memicmp(value, "true", 4) == 0};
	case PropertyType::Int8:
	case PropertyType::Int16:
	case PropertyType::Int32:
		src.int32 = strtol(value, nullptr, 0);
		break;
	case PropertyType::Int64:
		src.int64 = strtoll(value, nullptr, 0);
		break;
	case PropertyType::UInt8:
	case PropertyType::UInt16:
	case PropertyType::UInt32:
		src.uint32 = strtoul(value, nullptr, 0);
		break;
	case PropertyType::UInt64:
		src.uint64 = strtoull(value, nullptr, 0);
		break;
	case PropertyType::String:
		break;
	}

	PropertyData dst{};
	dst.setValue(prop, &src);
	return dst;
}

bool Store::writeCheck() const
{
	if(isLocked()) {
		return true;
	}
	debug_e("[CFGDB] Store is Read-only");
	return false;
}

void Store::queueUpdate(Object::UpdateCallback callback)
{
	return db.queueUpdate(*this, callback);
}

void Store::decUpdate()
{
	if(updaterCount == 0) {
		// No updaters: this happens where earlier call to `Database::lockStore()` failed
		return;
	}
	--updaterCount;
	CFGDB_DEBUG(" %u", updaterCount)
	if(updaterCount == 0) {
		commit();
		db.checkUpdateQueue(*this);
	}
}

bool Store::commit()
{
	if(!dirty) {
		return true;
	}

	if(!db.save(*this)) {
		return false;
	}

	dirty = false;
	return true;
}

} // namespace ConfigDB
