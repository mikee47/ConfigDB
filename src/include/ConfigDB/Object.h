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

#define CONFIGDB_OBJECT_TYPE_MAP(XX)                                                                                   \
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

/**
 * @brief Identifies array storage within array pool
 * @note alignas(1) required as value contained in packed structures
 */
using ArrayId = uint16_t alignas(1);

struct ObjectInfo {
	const FlashString& name;
	const ObjectInfo* parent;
	const ObjectInfo* const* objinfo;
	PGM_VOID_P defaultData;
	uint32_t structSize;
	ObjectType type;
	uint32_t objectCount;
	uint32_t propertyCount;
	const PropertyInfo propinfo[];

	static const ObjectInfo empty;

	/**
	 * @brief Root object in a store has no name
	 */
	bool isRoot() const
	{
		return name.length() == 0;
	}

	String getTypeDesc() const;

	/**
	 * @brief Get offset of this object's data relative to root (not just parent)
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
	Object() = default;

	explicit Object(Object& parent) : parent(&parent)
	{
	}

	virtual ~Object()
	{
	}

	String getPropertyValue(const PropertyInfo& prop, const void* data) const;

	bool setPropertyValue(const PropertyInfo& prop, void* data, const String& value);

	virtual Store& getStore()
	{
		assert(parent != nullptr);
		return parent->getStore();
	}

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
	virtual unsigned getObjectCount() const
	{
		return getTypeinfo().objectCount;
	}

	/**
	 * @brief Get child object by index
	 */
	virtual std::unique_ptr<Object> getObject(unsigned index) = 0;

	/**
	 * @brief Get number of properties
	 * @note Array types override this to return the number of items in the array.
	 */
	virtual unsigned getPropertyCount() const
	{
		return getTypeinfo().propertyCount;
	}

	/**
	 * @brief Get properties
	 * @note Array types override this to return array elements
	 */
	virtual Property getProperty(unsigned index)
	{
		auto& typeinfo = getTypeinfo();
		if(index >= typeinfo.propertyCount) {
			return {};
		}
		return {*this, typeinfo.propinfo[index], static_cast<uint8_t*>(getData()) + typeinfo.getPropertyOffset(index)};
	}

	/**
	 * @brief Commit changes to the store
	 */
	bool commit();

	// Printable [STOREIMPL]
	// virtual size_t printTo(Print& p) const = 0;

	virtual const ObjectInfo& getTypeinfo() const = 0;

	virtual void* getData() = 0;

	String getName() const
	{
		return getTypeinfo().name;
	}

	String getPath() const;

	size_t printTo(Print& p) const
	{
		// TODO
		return 0;
	}

protected:
	StringId addString(const String& value);

	Object* parent{};
};

/**
 * @brief Used by code generator
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 */
template <class ClassType> class ObjectTemplate : public Object
{
public:
	ObjectTemplate() = default;

	explicit ObjectTemplate(Object& parent) : Object(parent)
	{
	}

	const ObjectInfo& getTypeinfo() const override
	{
		return static_cast<const ClassType*>(this)->typeinfo;
	}
};

} // namespace ConfigDB

String toString(ConfigDB::ObjectType type);
