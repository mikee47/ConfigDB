/****
 * ConfigDB/ObjectRef.h
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

#pragma once

#include "Object.h"

namespace ConfigDB
{
/**
 * @brief Reference to an object independent of where it resides
 *
 * An Object contains 'dataref' which can be *either* an array index
 * *or* an offset. I *think* these are the cases:
 *
 * 1. Object is a direct child of store:
 *
 * 		- store
 * 			object
 *
 * 2. Object is an indirect child (intermediate objects):
 *
 * 		- store
 * 			parent (offset): Replaces one or more intermediate objects
 * 				object (offset)
 *
 * 3. Member of an ObjectArray:
 *
 * 		- store
 * 			ObjectArray
 * 				item (index)
 *
 * 4. Indirect child of object within ObjectArray:
 *
 * 		- store
 * 			ObjectArray
 *				item (index)
 * 					object (plus offset)
 * 
 * So we need a StoreRef plus three objects.
 */
struct ObjectRefBase {
	Object array;
	Object parent; ///< Either the store or an array item
	Object object;

	ObjectRefBase() = default;

	ObjectRefBase(const Object& object);

	ObjectRefBase(const ObjectRefBase& other)
	{
		copy(other);
	}

	ObjectRefBase(ObjectRefBase&&) = delete;

	explicit operator bool() const
	{
		return bool(object);
	}

protected:
	void copy(const ObjectRefBase& other);
};

struct ObjectRef : public ObjectRefBase {
	StoreRef store;

	using ObjectRefBase::ObjectRefBase;

	ObjectRef(StoreRef store);

	ObjectRef(StoreRef store, unsigned propIndex);

	ObjectRef(StoreRef store, const Object& object);

	ObjectRef(const ObjectRef& other);

	ObjectRef& operator=(const ObjectRef& other);
};

struct ObjectUpdateRef : public ObjectRefBase {
	StoreUpdateRef store;

	using ObjectRefBase::ObjectRefBase;

	ObjectUpdateRef(StoreUpdateRef store, Object& object);

	ObjectUpdateRef(const ObjectUpdateRef& other);

	ObjectUpdateRef& operator=(const ObjectUpdateRef& other);
};

} // namespace ConfigDB
