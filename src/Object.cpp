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

#include "include/ConfigDB/Database.h"
#include "include/ConfigDB/Json/Format.h"
#include <Data/Stream/FileStream.h>
#include <Data/Buffer/PrintBuffer.h>

namespace ConfigDB
{
Object& Object::operator=(const Object& other)
{
	propinfoPtr = other.propinfoPtr;
	parent = other.isStore() ? const_cast<Object*>(&other) : other.parent;
	dataRef = other.dataRef;
	streamPos = 0;
	return *this;
}

std::shared_ptr<Store> Object::openStore(Database& db, unsigned storeIndex, bool lockForWrite)
{
	return db.openStore(storeIndex, lockForWrite);
}

bool Object::lockStore(std::shared_ptr<Store>& store)
{
	if(store->isLocked()) {
		store->incUpdate();
		return true;
	}
	// Get root object which has pointer to Store: this may change
	auto obj = this;
	while(!obj->parent->typeIs(ObjectType::Store)) {
		obj = obj->parent;
	}
	assert(obj->parent);
	// Update store pointer
	if(!store->getDatabase().lockStore(store)) {
		return false;
	}
	obj->parent = store.get();
	return true;
}

void Object::unlockStore(Store& store)
{
	store.decUpdate();
}

Store& Object::getStore()
{
	auto obj = this;
	while(obj->parent) {
		obj = obj->parent;
	}
	assert(obj->typeinfo().type == ObjectType::Store);
	auto store = static_cast<Store*>(obj);
	return *store;
}

bool Object::isLocked() const
{
	return getStore().isLocked();
}

bool Object::writeCheck() const
{
	return getStore().writeCheck();
}

void* Object::getDataPtr()
{
	if(!writeCheck()) {
		return nullptr;
	}
	unsigned offset{0};
	auto obj = this;
	while(obj->parent) {
		if(obj->parent->isArray()) {
			auto array = static_cast<ArrayBase*>(obj->parent);
			return static_cast<uint8_t*>(array->getItem(obj->dataRef)) + offset;
		}
		offset += obj->dataRef + obj->propinfo().offset;
		obj = obj->parent;
	}
	assert(obj->typeIs(ObjectType::Store));
	return static_cast<Store*>(obj)->getRootData() + offset;
}

const void* Object::getDataPtr() const
{
	unsigned offset{0};
	auto obj = this;
	while(obj->parent) {
		if(obj->parent->isArray()) {
			auto array = static_cast<const ArrayBase*>(obj->parent);
			return static_cast<const uint8_t*>(array->getItem(obj->dataRef)) + offset;
		}
		offset += obj->dataRef + obj->propinfo().offset;
		obj = obj->parent;
	}
	assert(obj->typeIs(ObjectType::Store));
	return static_cast<const Store*>(obj)->getRootData() + offset;
}

unsigned Object::getObjectCount() const
{
	switch(typeinfo().type) {
	case ObjectType::ObjectArray:
		return static_cast<const ObjectArray*>(this)->getObjectCount();
	case ObjectType::Union:
		return static_cast<const Union*>(this)->getObjectCount();
	default:
		return typeinfo().objectCount;
	}
}

Object Object::getObject(unsigned index)
{
	switch(typeinfo().type) {
	case ObjectType::ObjectArray:
		return static_cast<ObjectArray*>(this)->getObject(index);
	case ObjectType::Union:
		return static_cast<Union*>(this)->getObject(index);
	default:
		return Object(*this, index);
	}
}

Object Object::findObject(const char* name, size_t length)
{
	if(isArray()) {
		return {};
	}
	int index = typeinfo().findObject(name, length);
	if(index < 0) {
		return {};
	}
	if(typeIs(ObjectType::Union)) {
		static_cast<Union*>(this)->setTag(index);
	}
	return Object(*this, index);
}

Property Object::findProperty(const char* name, size_t length)
{
	switch(typeinfo().type) {
	case ObjectType::Array:
	case ObjectType::ObjectArray:
	case ObjectType::Union:
		return {};
	default:
		int index = typeinfo().findProperty(name, length);
		return index >= 0 ? getProperty(index) : Property();
	}
}

void Object::queueUpdate(UpdateCallback callback)
{
	return getStore().queueUpdate(callback);
}

bool Object::commit()
{
	return getStore().commit();
}

void Object::clearDirty()
{
	getStore().clearDirty();
}

Database& Object::getDatabase()
{
	return getStore().getDatabase();
}

String Object::getName() const
{
	if(parent && parent->isArray()) {
		String path;
		path += '[';
		path += dataRef; // TODO: When items are deleted index will change, so use parent->getItemIndex(*this);
		path += ']';
		return path;
	}
	return propinfo().name;
}

String Object::getPath() const
{
	String path;
	if(parent) {
		path = parent->getPath();
	}
	String name = getName();
	if(path && name[0] != '[') {
		path += '.';
	}
	path += name;
	return path;
}

String Object::getPropertyString(unsigned index, StringId id) const
{
	if(id) {
		return String(getStore().stringPool[id]);
	}
	auto& prop = typeinfo().getProperty(index);
	if(prop.type == PropertyType::String && prop.defaultString) {
		return *prop.defaultString;
	}
	return nullptr;
}

String Object::getPropertyString(unsigned index) const
{
	auto data = getPropertyData(index);
	return data ? getPropertyString(index, data->string) : nullptr;
}

StringId Object::getStringId(const PropertyInfo& prop, const char* value, uint16_t valueLength)
{
	PropertyData dst{};
	auto defaultData = PropertyData::fromStruct(prop, getDataPtr());
	getStore().parseString(prop, dst, defaultData, value, valueLength);
	return dst.string;
}

void Object::setPropertyValue(unsigned index, const void* value)
{
	auto& prop = typeinfo().getProperty(index);
	auto data = PropertyData::fromStruct(prop, getDataPtr());
	if(!data) {
		return;
	}
	if(value) {
		auto src = static_cast<const PropertyData*>(value);
		data->setValue(prop, *src);
	} else {
		auto defaultData = PropertyData::fromStruct(prop, typeinfo().defaultData);
		memcpy_P(data, defaultData, prop.getSize());
	}
}

void Object::setPropertyValue(unsigned index, const String& value)
{
	auto& prop = typeinfo().getProperty(index);
	auto data = PropertyData::fromStruct(prop, getDataPtr());
	if(data) {
		data->string = getStringId(prop, value);
	}
}

unsigned Object::getPropertyCount() const
{
	switch(typeinfo().type) {
	case ObjectType::Array:
		return static_cast<const Array*>(this)->getPropertyCount();
	case ObjectType::Union:
		return static_cast<const Union*>(this)->getPropertyCount();
	default:
		return typeinfo().propertyCount;
	}
}

Property Object::getProperty(unsigned index)
{
	switch(typeinfo().type) {
	case ObjectType::Array:
		return static_cast<Array*>(this)->getProperty(index);
	case ObjectType::Union:
		return static_cast<Union*>(this)->getProperty(index);
	default:
		assert(index < typeinfo().propertyCount);
		if(index >= typeinfo().propertyCount) {
			return {};
		}
		auto& prop = typeinfo().getProperty(index);
		auto propData = getPropertyData(index);
		auto defaultData = PropertyData::fromStruct(prop, typeinfo().defaultData);
		return {getStore(), prop, propData, defaultData};
	}
}

PropertyConst Object::getProperty(unsigned index) const
{
	switch(typeinfo().type) {
	case ObjectType::Array:
		return static_cast<const Array*>(this)->getProperty(index);
	case ObjectType::Union:
		return static_cast<const Union*>(this)->getProperty(index);
	default:
		assert(index < typeinfo().propertyCount);
		if(index >= typeinfo().propertyCount) {
			return {};
		}
		auto& prop = typeinfo().getProperty(index);
		auto propData = getPropertyData(index);
		return {getStore(), prop, propData, nullptr};
	}
}

size_t Object::printTo(Print& p) const
{
	Json::Format format;
	format.setPretty(true);
	return format.exportToStream(*this, p);
}

bool Object::exportToFile(const Format& format, const String& filename) const
{
	createDirectories(filename);

	FileStream stream;
	if(stream.open(filename, File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<512> buffer(stream);
		exportToStream(format, buffer);
	}

	if(stream.getLastError() == FS_OK) {
		debug_d("[CFGDB] Object saved to '%s'", filename.c_str());
		return true;
	}

	debug_e("[CFGDB] Object save to '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
	return false;
}

Status Object::importFromFile(const Format& format, const String& filename)
{
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		if(stream.getLastError() == IFS::Error::NotFound) {
			return {};
		}
		debug_w("[CFGDB] open '%s' failed", filename.c_str());
		return Status::fileError(stream.getLastError());
	}

	return format.importFromStream(*this, stream);
}

} // namespace ConfigDB
