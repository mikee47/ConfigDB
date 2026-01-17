/****
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

namespace ConfigDB
{
uint8_t Store::instanceCount;

Store::Store(Database& db, const PropertyInfo& propinfo)
	: Object(propinfo), db(db), rootData(std::make_unique<uint8_t[]>(propinfo.variant.object->structSize))
{
	auto& obj = *propinfo.variant.object;
	if(obj.type == ObjectType::Array) {
		*reinterpret_cast<ArrayId*>(rootData.get()) = arrayPool.add(obj);
	} else if(obj.defaultData) {
		memcpy_P(rootData.get(), obj.defaultData, obj.structSize);
	}
	++instanceCount;
	CFGDB_DEBUG(" %u", instanceCount)
}

Store::Store(const Store& store)
	: Object(store.propinfo()), arrayPool(store.arrayPool), stringPool(store.stringPool), db(store.db),
	  rootData(std::make_unique<uint8_t[]>(store.typeinfo().structSize))
{
	++instanceCount;
	CFGDB_DEBUG(" COPY %u", instanceCount)
	memcpy(rootData.get(), store.rootData.get(), typeinfo().structSize);
}

Store::~Store()
{
	if(*this) {
		--instanceCount;
		CFGDB_DEBUG(" %u", instanceCount);
	}
}

void Store::clear()
{
	if(!writeCheck()) {
		return;
	}

	stringPool.clear();
	Object::clear();
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
	if(info.type == PropertyType::String) {
		if(propData.string) {
			return String(stringPool[propData.string]);
		}
		if(info.variant.defaultString) {
			return *info.variant.defaultString;
		}
		return nullptr;
	}

	return propData.getString(info);
}

bool Store::parseString(const PropertyInfo& prop, PropertyData& dst, const PropertyData* defaultData, const char* value,
						uint16_t valueLength)
{
	if(prop.type == PropertyType::String) {
		if(!value) {
			dst.string = 0;
		} else if(prop.variant.defaultString && *prop.variant.defaultString == value) {
			dst.string = 0;
		} else {
			dst.string = stringPool.findOrAdd({value, valueLength});
		}
		return true;
	}

	if(!value && defaultData) {
		memcpy_P(&dst.int8, defaultData, prop.getSize());
		return true;
	}

	return dst.setValue(prop, value, valueLength);
}

bool Store::writeCheck() const
{
	if(isLocked()) {
		return true;
	}
	debug_e("[CFGDB] Store is Read-only");
	return false;
}

void Store::queueUpdate(Object::UpdateCallback&& callback)
{
	return db.queueUpdate(*this, std::move(callback));
}

void Store::checkRef(const StoreRef& ref)
{
	db.checkStoreRef(ref);
}

void Store::incUpdate()
{
	++updaterCount;
	CFGDB_DEBUG(" %u", updaterCount)
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
