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
#include <Printable.h>
#include <memory>

#include <debug_progmem.h>

#define CONFIGDB_OBJECT_TYPE_MAP(XX)                                                                                   \
	XX(Store)                                                                                                          \
	XX(Object)                                                                                                         \
	XX(Array)                                                                                                          \
	XX(ObjectArray)

namespace ConfigDB
{
class Database;
class Store;

enum class ObjectType : uint32_t {
#define XX(name) name,
	CONFIGDB_OBJECT_TYPE_MAP(XX)
#undef XX
};

String toString(ObjectType type);

/**
 * @brief Identifies array storage within array pool
 * @note alignas(1) required as value contained in packed structures
 */
using ArrayId = uint16_t alignas(1);

struct ObjectInfo {
	ObjectType type;
	const FlashString& name;
	const ObjectInfo* parent;
	const ObjectInfo* const* objinfo;
	PGM_VOID_P defaultData;
	uint32_t structSize;
	uint32_t objectCount;
	uint32_t propertyCount;
	const PropertyInfo propinfo[];

	static const ObjectInfo empty;

	ObjectInfo(const ObjectInfo&) = delete;

	String getTypeDesc() const;

	/**
	 * @brief Get offset of this object's data relative to its parent
	 */
	size_t getOffset() const;

	/**
	 * @brief Get offset of data for a property from the start of *this* object's data
	 */
	size_t getPropertyOffset(unsigned index) const;
};

static_assert(sizeof(ObjectInfo) == 32, "Bad ObjectInfo size");

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

	explicit Object(const ObjectInfo& typeinfo) : typeinfoPtr(&typeinfo)
	{
	}

	Object(const ObjectInfo& typeinfo, Store& store);

	Object(const ObjectInfo& typeinfo, Object* parent, void* data) : typeinfoPtr(&typeinfo), parent(parent), data(data)
	{
	}

	explicit operator bool() const
	{
		return typeinfoPtr != &ObjectInfo::empty;
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

	PropertyConst getProperty(unsigned index) const
	{
		return const_cast<Object*>(this)->getProperty(index);
	}

	/**
	 * @brief Find property by name
	 */
	Property findProperty(const char* name, size_t length);

	/**
	 * @brief Commit changes to the store
	 */
	bool commit();

	String getName() const
	{
		return typeinfo().name;
	}

	String getPath() const;

	size_t printTo(Print& p) const;

	const ObjectInfo& typeinfo() const
	{
		return *typeinfoPtr;
	}

protected:
	std::shared_ptr<Store> openStore(Database& db, const ObjectInfo& typeinfo);

	String getString(StringId id) const;

	StringId getStringId(const char* value, size_t valueLength);

	StringId getStringId(const String& value)
	{
		return getStringId(value.c_str(), value.length());
	}

	const ObjectInfo* typeinfoPtr;
	Object* parent{};
	void* data{};
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator
 */
template <class ClassType> class ObjectTemplate : public Object
{
public:
	ObjectTemplate(const ObjectInfo& typeinfo, std::shared_ptr<Store> store) : Object(typeinfo, store)
	{
	}

	explicit ObjectTemplate(Store& store) : Object(ClassType::typeinfo, store)
	{
	}

	ObjectTemplate(Object& parent, void* data) : Object(ClassType::typeinfo, &parent, data)
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
	OuterObjectTemplate(std::shared_ptr<Store> store) : ContainedClassType(*store), store(store)
	{
	}

	OuterObjectTemplate(Database& db) : OuterObjectTemplate(this->openStore(db, StoreType::typeinfo))
	{
	}

private:
	std::shared_ptr<ConfigDB::Store> store;
};

} // namespace ConfigDB
