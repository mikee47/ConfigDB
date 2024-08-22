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
#include "include/ConfigDB/Json/Reader.h"

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

std::shared_ptr<Store> Object::openStore(Database& db, const ObjectInfo& typeinfo, bool forWrite)
{
	return db.openStore(typeinfo, forWrite);
}

bool Object::unlockStore(std::shared_ptr<Store>& store)
{
	return store->getDatabase().unlock(store);
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

bool Object::isReadOnly() const
{
	return getStore().isReadOnly();
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
		assert(typeinfo().type == ObjectType::Store);
		return static_cast<Store*>(this)->getRootData();
	}
	switch(parent->typeinfo().type) {
	case ObjectType::Array:
	case ObjectType::ObjectArray:
		return static_cast<ArrayBase*>(parent)->getItem(dataRef);
	default:
		return static_cast<uint8_t*>(parent->getDataPtr()) + dataRef;
	}
}

const void* Object::getDataPtr() const
{
	if(!parent) {
		assert(typeinfo().type == ObjectType::Store);
		return static_cast<const Store*>(this)->getRootData();
	}
	switch(parent->typeinfo().type) {
	case ObjectType::Array:
	case ObjectType::ObjectArray:
		return static_cast<const ArrayBase*>(parent)->getItem(dataRef);
	default:
		auto data = static_cast<const Object*>(parent)->getDataPtr();
		return static_cast<const uint8_t*>(data) + dataRef;
	}
}

unsigned Object::getObjectCount() const
{
	if(typeinfo().type == ObjectType::ObjectArray) {
		return static_cast<const ObjectArray*>(this)->getObjectCount();
	}
	return typeinfo().objectCount;
}

Object Object::getObject(unsigned index)
{
	if(typeinfo().type == ObjectType::ObjectArray) {
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
	if(typeinfo().type > ObjectType::Object) {
		return {};
	}
	int i = typeinfo().findObject(name, length);
	return i >= 0 ? getObject(i) : Object();
}

Property Object::findProperty(const char* name, size_t length)
{
	if(typeinfo().type > ObjectType::Object) {
		return {};
	}
	int i = typeinfo().findProperty(name, length);
	return i >= 0 ? getProperty(i) : Property();
}

bool Object::commit()
{
	auto& store = getStore();
	if(store.isReadOnly()) {
		return false;
	}
	return store.getDatabase().save(store);
}

Database& Object::getDatabase()
{
	return getStore().getDatabase();
}

String Object::getName() const
{
	if(parent && (parent->typeinfo().type == ObjectType::Array || parent->typeinfo().type == ObjectType::ObjectArray)) {
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

String Object::getString(StringId id) const
{
	return String(getStore().stringPool[id]);
}

StringId Object::getStringId(const char* value, uint16_t valueLength)
{
	return getStore().stringPool.findOrAdd({value, valueLength});
}

unsigned Object::getPropertyCount() const
{
	if(typeinfo().type == ObjectType::Array) {
		return static_cast<const Array*>(this)->getPropertyCount();
	}
	return typeinfo().propertyCount;
}

Property Object::getProperty(unsigned index)
{
	if(typeinfo().type == ObjectType::Array) {
		return static_cast<Array*>(this)->getProperty(index);
	}

	if(index >= typeinfo().propertyCount) {
		return {};
	}
	auto propData = getData<uint8_t>();
	propData += typeinfo().getPropertyOffset(index);
	return {getStore(), typeinfo().propinfo[index], propData};
}

PropertyConst Object::getProperty(unsigned index) const
{
	if(typeinfo().type == ObjectType::Array) {
		return static_cast<const Array*>(this)->getProperty(index);
	}

	if(index >= typeinfo().propertyCount) {
		return {};
	}
	auto propData = getData<const uint8_t>();
	propData += typeinfo().getPropertyOffset(index);
	return {getStore(), typeinfo().propinfo[index], propData};
}

size_t Object::printTo(Print& p) const
{
	Json::Reader reader;
	reader.setFormat(Json::Format::Pretty);
	return reader.saveToStream(*this, p);
}

} // namespace ConfigDB
