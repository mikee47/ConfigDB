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
Object::Object(const ObjectInfo& typeinfo, Store& store) : Object(typeinfo, store, store.getObjectDataRef(typeinfo))
{
}

Object& Object::operator=(const Object& other)
{
	typeinfoPtr = other.typeinfoPtr;
	parent = other.isStore() ? const_cast<Object*>(&other) : other.parent;
	dataRef = other.dataRef;
	streamPos = 0;
	return *this;
}

std::shared_ptr<Store> Object::openStore(Database& db, const ObjectInfo& typeinfo, bool lockForWrite)
{
	return db.openStore(typeinfo, lockForWrite);
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
	if(!parent) {
		assert(typeIs(ObjectType::Store));
		return static_cast<Store*>(this)->getRootData();
	}
	if(parent->isArray()) {
		return static_cast<ArrayBase*>(parent)->getItem(dataRef);
	}
	return static_cast<uint8_t*>(parent->getDataPtr()) + dataRef;
}

const void* Object::getDataPtr() const
{
	if(!parent) {
		assert(typeIs(ObjectType::Store));
		return static_cast<const Store*>(this)->getRootData();
	}
	if(parent->isArray()) {
		return static_cast<const ArrayBase*>(parent)->getItem(dataRef);
	}
	auto data = static_cast<const Object*>(parent)->getDataPtr();
	return static_cast<const uint8_t*>(data) + dataRef;
}

unsigned Object::getObjectCount() const
{
	if(typeIs(ObjectType::ObjectArray)) {
		return static_cast<const ObjectArray*>(this)->getObjectCount();
	}
	return typeinfo().objectCount;
}

Object Object::getObject(unsigned index)
{
	if(typeIs(ObjectType::ObjectArray)) {
		return static_cast<ObjectArray*>(this)->getObject(index);
	}

	if(index >= typeinfo().objectCount) {
		return {};
	}

	auto typ = typeinfo().objinfo[index];
	return Object(*typ, *this, typ->getOffset());
}

Object Object::findObject(const char* name, size_t length)
{
	if(isArray()) {
		return {};
	}
	int i = typeinfo().findObject(name, length);
	return i >= 0 ? getObject(i) : Object();
}

Property Object::findProperty(const char* name, size_t length)
{
	if(isArray()) {
		return {};
	}
	int i = typeinfo().findProperty(name, length);
	return i >= 0 ? getProperty(i) : Property();
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
	return typeinfo().name;
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

String Object::getString(const PropertyInfo& prop, StringId id) const
{
	if(id) {
		return String(getStore().stringPool[id]);
	}
	return prop.defaultValue ? String(*prop.defaultValue) : nullptr;
}

StringId Object::getStringId(const char* value, uint16_t valueLength)
{
	return value ? getStore().stringPool.findOrAdd({value, valueLength}) : 0;
}

unsigned Object::getPropertyCount() const
{
	if(typeIs(ObjectType::Array)) {
		return static_cast<const Array*>(this)->getPropertyCount();
	}
	return typeinfo().propertyCount;
}

Property Object::getProperty(unsigned index)
{
	if(typeIs(ObjectType::Array)) {
		return static_cast<Array*>(this)->getProperty(index);
	}

	if(index >= typeinfo().propertyCount) {
		return {};
	}
	auto propData = getData<uint8_t>();
	if(propData) {
		propData += typeinfo().getPropertyOffset(index);
	}
	return {getStore(), typeinfo().propinfo[index], propData};
}

PropertyConst Object::getProperty(unsigned index) const
{
	if(typeIs(ObjectType::Array)) {
		return static_cast<const Array*>(this)->getProperty(index);
	}

	if(index >= typeinfo().propertyCount) {
		return {};
	}
	auto propData = getData<const uint8_t>();
	if(propData) {
		propData += typeinfo().getPropertyOffset(index);
	}
	return {getStore(), typeinfo().propinfo[index], propData};
}

size_t Object::printTo(Print& p) const
{
	Json::Format format;
	format.setPretty(true);
	return format.exportToStream(*this, p);
}

bool Object::exportToFile(const Format& format, const String& filename) const
{
	FileStream stream;
	if(stream.open(filename, File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<512> buffer(stream);
		exportToStream(format, buffer);
	}

	if(stream.getLastError() == FS_OK) {
		debug_d("[JSON] Store saved '%s' OK", filename.c_str());
		return true;
	}

	debug_e("[JSON] Store save '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
	return false;
}

bool Object::importFromFile(const Format& format, const String& filename)
{
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		if(stream.getLastError() == IFS::Error::NotFound) {
			return true;
		}
		debug_w("open('%s') failed", filename.c_str());
		return false;
	}

	return format.importFromStream(*this, stream);
}

} // namespace ConfigDB
