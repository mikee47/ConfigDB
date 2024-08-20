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

namespace ConfigDB
{
const ObjectInfo PROGMEM ObjectInfo::empty{.name = fstr_empty};

String toString(ObjectType type)
{
	switch(type) {
#define XX(name)                                                                                                       \
	case ObjectType::name:                                                                                             \
		return F(#name);
		CONFIGDB_OBJECT_TYPE_MAP(XX)
#undef XX
	}
	return nullptr;
}

size_t ObjectInfo::getOffset() const
{
	if(!parent) {
		return 0;
	}
	size_t offset{0};
	for(unsigned i = 0; i < parent->objectCount; ++i) {
		auto obj = parent->objinfo[i];
		if(obj == this) {
			return offset;
		}
		offset += obj->structSize;
	}
	assert(false);
	return 0;
}

size_t ObjectInfo::getPropertyOffset(unsigned index) const
{
	size_t offset{0};
	for(unsigned i = 0; i < objectCount; ++i) {
		offset += objinfo[i]->structSize;
	}
	for(unsigned i = 0; i < propertyCount; ++i) {
		if(i == index) {
			return offset;
		}
		offset += propinfo[i].getSize();
	}
	assert(false);
	return 0;
}

int ObjectInfo::findObject(const char* name, size_t length) const
{
	for(unsigned i = 0; i < objectCount; ++i) {
		if(objinfo[i]->name.equals(name, length)) {
			return i;
		}
	}
	return -1;
}

int ObjectInfo::findProperty(const char* name, size_t length) const
{
	for(unsigned i = 0; i < propertyCount; ++i) {
		if(propinfo[i].name.equals(name, length)) {
			return i;
		}
	}
	return -1;
}

String ObjectInfo::getTypeDesc() const
{
	String s;
	s += toString(type);
	if(type == ObjectType::Array) {
		s += '[';
		s += toString(propinfo[0].type);
		s += ']';
	} else if(type == ObjectType::ObjectArray) {
		s += "[Object]";
	}
	return s;
}

Object::Object(const ObjectInfo& typeinfo, Store& store) : Object(typeinfo, &store, store.getObjectDataRef(typeinfo))
{
}

Object& Object::operator=(const Object& other)
{
	typeinfoPtr = other.typeinfoPtr;
	parent = other.isStore() ? const_cast<Object*>(&other) : other.parent;
	dataRef = other.dataRef;
	return *this;
}

std::shared_ptr<Store> Object::openStore(Database& db, const ObjectInfo& typeinfo)
{
	return db.openStore(typeinfo);
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

void* Object::getData()
{
	if(!parent) {
		assert(typeinfo().type == ObjectType::Store);
		return static_cast<Store*>(this)->getRootData();
	}
	switch(parent->typeinfo().type) {
	case ObjectType::Array:
	case ObjectType::ObjectArray:
		return static_cast<ArrayBase*>(parent)->getItem(dataRef);
	default:
		return static_cast<uint8_t*>(parent->getData()) + dataRef;
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
	return Object(*typ, this, typ->getOffset());
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
	return getStore().save();
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
	return getStore().stringPool[id];
}

StringId Object::getStringId(const char* value, size_t valueLength)
{
	return getStore().stringPool.findOrAdd(value, valueLength);
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
	auto propData = static_cast<uint8_t*>(getData());
	propData += typeinfo().getPropertyOffset(index);
	return {getStore(), typeinfo().propinfo[index], propData};
}

size_t Object::printTo(Print& p) const
{
	return getStore().printObjectTo(*this, &typeinfo().name, 0, p);
}

} // namespace ConfigDB
