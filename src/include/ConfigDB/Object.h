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
#include "Format.h"

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

	bool typeIs(ObjectType type) const
	{
		return *this && typeinfo().type == type;
	}

	bool isArray() const
	{
		return typeinfo().isArray();
	}

	/**
	 * @brief Determine if this object *is* a store (not just a reference to it)
	 */
	bool isStore() const
	{
		return typeinfoPtr->type == ObjectType::Store && !parent;
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

	const Object getObject(unsigned index) const
	{
		return const_cast<Object*>(this)->getObject(index);
	}

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

	/**
	 * @brief Clear store dirty flag so changes don't get committed
	 * @note Store must be reloaded to roll back any changes
	 */
	void clearDirty();

	String getName() const;

	String getPath() const;

	size_t printTo(Print& p) const;

	bool exportToStream(const Format& format, Print& output) const
	{
		return format.exportToStream(*this, output);
	}

	bool exportToFile(const Format& format, const String& filename) const;

	Status importFromStream(const Format& format, Stream& source)
	{
		return format.importFromStream(*this, source);
	}

	Status importFromFile(const Format& format, const String& filename);

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

	using UpdateCallback = Delegate<void(Store& store)>;

	void queueUpdate(UpdateCallback callback);

protected:
	std::shared_ptr<Store> openStore(Database& db, const ObjectInfo& typeinfo, bool lockForWrite = false);

	bool isLocked() const;

	bool isWriteable() const
	{
		if(isLocked()) {
			assert(getDataPtr());
		}
		return isLocked() && getDataPtr();
	}

	bool lockStore(std::shared_ptr<Store>& store);

	void unlockStore(Store& store);

	bool writeCheck() const;

	void* getDataPtr();

	const void* getDataPtr() const;

	String getString(const PropertyInfo& prop, StringId id) const;

	StringId getStringId(const char* value, uint16_t valueLength);

	StringId getStringId(const String& value)
	{
		return value ? getStringId(value.c_str(), value.length()) : 0;
	}

	template <typename T> StringId getStringId(const T& value)
	{
		return getStringId(toString(value));
	}

	void setPropertyValue(const PropertyInfo& prop, uint16_t offset, const void* value);
	void setPropertyValue(const PropertyInfo& prop, uint16_t offset, const String& value);

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

	ObjectTemplate(const Object& parent, uint16_t dataRef)
		: Object(ClassType::typeinfo, const_cast<Object&>(parent), dataRef)
	{
	}
};

/**
 * @brief Used by code generator
 * @tparam UpdaterType
 * @tparam ClassType Contained class with type information
 */
template <class UpdaterType, class ClassType> class ObjectUpdaterTemplate : public ClassType
{
public:
	using ClassType::ClassType;

	explicit operator bool() const
	{
		return Object::operator bool() && this->isWriteable();
	}
};

/**
 * @brief Used by code generator
 * @tparam UpdaterType
 * @tparam StoreType
 */
template <class UpdaterType, class StoreType> class OuterObjectUpdaterTemplate : public UpdaterType
{
public:
	OuterObjectUpdaterTemplate(std::shared_ptr<Store> store) : UpdaterType(*store), store(store)
	{
	}

	explicit OuterObjectUpdaterTemplate(Database& db)
		: OuterObjectUpdaterTemplate(this->openStore(db, StoreType::typeinfo, true))
	{
	}

	~OuterObjectUpdaterTemplate()
	{
		this->unlockStore(*store);
	}

	std::unique_ptr<ImportStream> createImportStream(const Format& format)
	{
		return format.createImportStream(store, *this);
	}

private:
	std::shared_ptr<Store> store;
};

/**
 * @brief Used by code generator
 * @tparam ContainedClassType
 * @tparam UpdaterType
 * @tparam StoreType
 */
template <class ContainedClassType, class UpdaterType, class StoreType>
class OuterObjectTemplate : public ContainedClassType
{
public:
	OuterObjectTemplate(std::shared_ptr<Store> store) : ContainedClassType(*store), store(store)
	{
	}

	explicit OuterObjectTemplate(Database& db) : OuterObjectTemplate(this->openStore(db, StoreType::typeinfo))
	{
	}

	std::unique_ptr<ExportStream> createExportStream(const Format& format) const
	{
		return format.createExportStream(store, *this);
	}

	using OuterUpdater = OuterObjectUpdaterTemplate<UpdaterType, StoreType>;

	/**
	 * @brief Create an update object
	 * @retval Updater Instance to allow setting values
	 * Caller **must** check validity of returned updater as update may already be in progress.
	 *
	 * 		if (auto upd = myobject.update()) {
	 * 			// OK, proceed with update
	 * 			upd.setValue(...);
	 *		} else {
	 *			// Cannot update at the moment
	 *		}
	 */
	OuterUpdater update()
	{
		this->lockStore(store);
		return OuterUpdater(store);
	}

	/**
	 * @brief Run an update asynchronously
	 * @param callback User callback which will receive an updater instance
	 * @retval bool true if update was performed immediately, false if it's been queued
	 */
	bool update(Delegate<void(UpdaterType)> callback)
	{
		if(auto upd = update()) {
			callback(upd);
			return true;
		}
		Object::queueUpdate([this, callback](Store& store) { callback(UpdaterType(store)); });
		return false;
	}

private:
	std::shared_ptr<Store> store;
};

} // namespace ConfigDB
