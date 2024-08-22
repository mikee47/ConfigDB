/**
 * ConfigDB/Object.h
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

#pragma once

#include "Property.h"
#include "ObjectInfo.h"
#include <memory>

namespace ConfigDB
{
class Database;
class Store;

/**
 * @brief An object can contain other objects, properties and arrays
 * @note This class is the base for concrete Object, Array and ObjectArray classes
 */
class Object
{
public:
	Object() : typeinfoPtr(&ObjectInfo::empty)
	{
	}

	Object(const Object& other)
	{
		*this = other;
	}

	Object(Object&&) = delete;

	Object& operator=(const Object& other);

	explicit Object(const ObjectInfo& typeinfo) : typeinfoPtr(&typeinfo)
	{
	}

	Object(const ObjectInfo& typeinfo, Store& store);

	Object(const ObjectInfo& typeinfo, Object& parent, uint16_t dataRef)
		: typeinfoPtr(&typeinfo), parent(&parent), dataRef(dataRef)
	{
	}

	explicit operator bool() const
	{
		return typeinfoPtr != &ObjectInfo::empty;
	}

	/**
	 * @brief Determine if this object *is* a store (not just a reference to it)
	 */
	bool isStore() const
	{
		return typeinfoPtr->type == ObjectType::Store && !parent && typeinfoPtr != &ObjectInfo::empty;
	}

	Store& getStore();

	const Store& getStore() const
	{
		return const_cast<Object*>(this)->getStore();
	}

	Database& getDatabase();

	const Database& getDatabase() const
	{
		return const_cast<Object*>(this)->getDatabase();
	}

	/**
	 * @brief Get number of child objects
	 * @note ObjectArray overrides this to return number of items in the array
	 */
	unsigned getObjectCount() const;

	/**
	 * @brief Get child object by index
	 */
	Object getObject(unsigned index);

	/**
	 * @brief Find child object by name
	 */
	Object findObject(const char* name, size_t length);

	/**
	 * @brief Get number of properties
	 * @note Array types override this to return the number of items in the array.
	 */
	unsigned getPropertyCount() const;

	/**
	 * @brief Get properties
	 * @note Array types override this to return array elements
	 */
	Property getProperty(unsigned index);

	PropertyConst getProperty(unsigned index) const;

	/**
	 * @brief Find property by name
	 */
	Property findProperty(const char* name, size_t length);

	/**
	 * @brief Commit changes to the store
	 */
	bool commit();

	String getName() const;

	String getPath() const;

	size_t printTo(Print& p) const;

	const ObjectInfo& typeinfo() const
	{
		return *typeinfoPtr;
	}

	template <typename T> T* getData()
	{
		return static_cast<T*>(getDataPtr());
	}

	template <typename T> const T* getData() const
	{
		return static_cast<const T*>(getDataPtr());
	}

protected:
	std::shared_ptr<Store> openStore(Database& db, const ObjectInfo& typeinfo, bool lockForWrite = false);

	bool isReadOnly() const;

	bool isWriteable() const
	{
		return !isReadOnly() && getDataPtr();
	}

	bool lockStore(std::shared_ptr<Store>& store);

	void unlockStore(Store& store);

	bool writeCheck() const;

	void* getDataPtr();

	const void* getDataPtr() const;

	String getString(StringId id) const;

	StringId getStringId(const char* value, uint16_t valueLength);

	StringId getStringId(const String& value)
	{
		return getStringId(value.c_str(), value.length());
	}

	template <typename T> StringId getStringId(const T& value)
	{
		return getStringId(toString(value));
	}

	const ObjectInfo* typeinfoPtr;
	Object* parent{};
	uint16_t dataRef{}; //< Relative to parent

public:
	uint16_t streamPos{}; //< Used during streaming
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator
 */
template <class ClassType> class ObjectTemplate : public Object
{
public:
	ObjectTemplate() : Object(ClassType::typeinfo)
	{
	}

	explicit ObjectTemplate(Store& store) : Object(ClassType::typeinfo, store)
	{
	}

	ObjectTemplate(Object& parent, uint16_t dataRef) : Object(ClassType::typeinfo, parent, dataRef)
	{
	}
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator
 * @tparam StoreType Object type for store root
 */
template <class ContainedClassType, class StoreType> class OuterObjectTemplate : public ContainedClassType
{
public:
	using Updater = typename ContainedClassType::Updater;

	OuterObjectTemplate(std::shared_ptr<Store> store) : ContainedClassType(*store), store(store)
	{
	}

	explicit OuterObjectTemplate(Database& db) : OuterObjectTemplate(this->openStore(db, StoreType::typeinfo))
	{
	}

	Updater beginUpdate()
	{
		this->lockStore(store);
		return Updater(store);
	}

private:
	std::shared_ptr<ConfigDB::Store> store;
};

/**
 * @brief Used by code generator
 * @tparam ClassType Outer class type provided by code generator
 */
template <class ClassType, class StoreType> class ObjectUpdaterTemplate : public Object
{
public:
	ObjectUpdaterTemplate(std::shared_ptr<Store> store) : Object(ClassType::typeinfo, *store), store(store)
	{
	}

	explicit ObjectUpdaterTemplate(Database& db) : ObjectUpdaterTemplate(this->openStore(db, StoreType::typeinfo, true))
	{
	}

	~ObjectUpdaterTemplate()
	{
		commit();
		this->unlockStore(*store);
	}

	explicit operator bool() const
	{
		return Object::operator bool() && isWriteable();
	}

private:
	std::shared_ptr<ConfigDB::Store> store;
};

} // namespace ConfigDB
