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

#define CONFIGDB_OBJECT_TYPE_MAP(XX)                                                                                   \
	XX(Object)                                                                                                         \
	XX(Array)                                                                                                          \
	XX(ObjectArray)

namespace ConfigDB
{
class Database;
class Store;

enum class ObjectType {
#define XX(name) name,
	CONFIGDB_OBJECT_TYPE_MAP(XX)
#undef XX
};

struct ObjectInfo {
	// DO NOT access these directly!
	const FlashString* name; ///< Within store, root always nullptr
	const FlashString* path; ///< Relative to store
	const FSTR::Vector<ObjectInfo>* objinfo;
	const FSTR::Array<PropertyInfo>* propinfo;
	ObjectType type;

	static const ObjectInfo& empty()
	{
		static const ObjectInfo PROGMEM emptyInfo{};
		return emptyInfo;
	}

	String getName() const
	{
		return name ? String(*name) : nullptr;
	}

	String getPath() const
	{
		return path ? String(*path) : nullptr;
	}

	ObjectType getType() const
	{
		return FSTR::readValue(&type);
	}
};

/**
 * @brief An object can contain other objects, properties and arrays
 * @note This class is the base for concrete Object, Array and ObjectArray classes
 */
class Object : public Printable
{
public:
	Object() = default;

	explicit Object(Object& parent) : parent(&parent)
	{
	}

	/**
	 * @brief Retrieve a value
	 * @param key Key for value
	 * @retval String
	 */
	virtual String getStoredValue(const String& key) const = 0;

	/**
	 * @brief Retrieve a value from an array
	 * @param key Index of value
	 * @retval String
	 */
	virtual String getStoredArrayValue(unsigned index) const = 0;

	/**
	 * @brief Store a value
	 * @param key Key for value
	 * @param value Value to store
	 * @retval bool true on success
	 */
	// virtual bool setStringValue(const String& key, const String& value) = 0;

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
	 */
	virtual unsigned getObjectCount() const = 0;

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
		auto& typeinfo = getTypeinfo();
		return typeinfo.propinfo ? typeinfo.propinfo->length() : 0;
	}

	/**
	 * @brief Get properties
	 * @note Array types override this to return array elements
	 */
	virtual Property getProperty(unsigned index)
	{
		auto& typeinfo = getTypeinfo();
		if(!typeinfo.propinfo || index >= typeinfo.propinfo->length()) {
			return {};
		}
		return {*this, *(typeinfo.propinfo->data() + index)};
	}

	/**
	 * @brief Commit changes to the store
	 */
	bool commit();

	// Printable [STOREIMPL]
	// virtual size_t printTo(Print& p) const = 0;

	virtual const ObjectInfo& getTypeinfo() const = 0;

	String getName() const
	{
		auto& typeinfo = getTypeinfo();
		return typeinfo.name ? String(*typeinfo.name) : nullptr;
	}

	String getPath() const;

protected:
	Object* parent{};
};

/**
 * @brief Used by store implemention to create specific template for `Object` only (not array)
 * @tparam BaseType The store's base `Object` class
 * @tparam ClassType Concrete type provided by code generator (CRTP)
 */
template <class BaseType, class ClassType> class ObjectTemplate : public BaseType
{
public:
	using BaseType::BaseType;

	const ObjectInfo& getTypeinfo() const override
	{
		return static_cast<const ClassType*>(this)->typeinfo;
	}

	String getStoredArrayValue(unsigned index) const override
	{
		return nullptr;
	}
};

} // namespace ConfigDB

String toString(ConfigDB::ObjectType type);
