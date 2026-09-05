/****
 * ConfigDB/ObjectRef.cpp
 *
 * Copyright 2026 mikee47 <mike@sillyhouse.net>
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

#include "include/ConfigDB/ObjectRef.h"
#include "include/ConfigDB/Store.h"

namespace ConfigDB
{
ObjectRefBase::ObjectRefBase(const Object& object)
{
	if(object.parent->isStore()) {
		this->object = object;
		return;
	}

	uint16_t offset{0};
	auto obj = &object;
	while(obj->parent) {
		if(obj->parent->isArray() && !obj->parent->isStore()) {
			this->array = *obj->parent;
			this->parent = Object(this->array, obj->propinfo(), obj->dataRef);
			this->object = Object(this->parent, object.propinfo(), offset);
			return;
		}
		offset += obj->dataRef;
		obj = obj->parent;
	}
	this->parent = *object.parent;
	this->object = Object(this->parent, object.propinfo(), offset);
}

void ObjectRefBase::copy(const ObjectRefBase& other)
{
	array = other.array;
	parent = other.parent;
	object = other.object;

	if(other.parent.parent == &other.array) {
		parent.parent = &array;
	}
	if(other.object.parent == &other.parent) {
		object.parent = &parent;
	}
}

ObjectRef::ObjectRef(StoreRef store) : ObjectRefBase(*store), store(store)
{
}

ObjectRef::ObjectRef(StoreRef store, unsigned propIndex) : ObjectRef(store, {*store, propIndex})
{
}

ObjectRef::ObjectRef(StoreRef store, const Object& object) : ObjectRefBase(object), store(store)
{
}

ObjectRef::ObjectRef(const ObjectRef& other) : ObjectRefBase(other), store(other.store)
{
}

ObjectRef& ObjectRef::operator=(const ObjectRef& other)
{
	store = other.store;
	copy(other);
	return *this;
}

ObjectUpdateRef::ObjectUpdateRef(const ObjectUpdateRef& other) : ObjectRefBase(other), store(other.store)
{
}

ObjectUpdateRef& ObjectUpdateRef::operator=(const ObjectUpdateRef& other)
{
	store = other.store;
	copy(other);
	return *this;
}

ObjectUpdateRef::ObjectUpdateRef(StoreUpdateRef store, Object& object) : ObjectRefBase(object), store(store)
{
}

} // namespace ConfigDB
