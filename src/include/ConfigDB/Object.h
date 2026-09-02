/****
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
#include "StoreRef.h"
#include "Format.h"

namespace ConfigDB
{
class Database;
class Store;
class ObjectRef;
class ObjectUpdateRef;

/**
 * @brief Callback invoked by asynchronous updater or other trigger points
 * @param store Store instance
 */
using Callback = Delegate<void(Store& store)>;

enum class CallbackType {
	update, ///< Application requested a one-time asynchronous update
	commit, ///< Invoked before committing changes to store
};

/**
 * @brief An object can contain other objects, properties and arrays
 * @note This class is the base for concrete Object, Array and ObjectArray classes
 */
class Object
{
public:
	Object() : propinfoPtr(&PropertyInfo::empty)
	{
	}

	Object(const Object& other)
	{
		*this = other;
	}

	Object(Object&&) = delete;

	Object& operator=(const Object& other);

	explicit Object(const PropertyInfo& propinfo) : propinfoPtr(&propinfo)
	{
	}

	Object(Object& parent, const PropertyInfo& prop, uint16_t dataRef)
		: propinfoPtr(&prop), parent(&parent), dataRef(dataRef)
	{
	}

	Object(const Object& parent, const PropertyInfo& prop, uint16_t dataRef)
		: Object(const_cast<Object&>(parent), prop, dataRef)
	{
	}

	Object(Object& parent, unsigned propIndex, uint16_t dataRef = 0)
		: Object(parent, parent.typeinfo().getObject(propIndex), dataRef)
	{
	}

	Object(const Object& parent, unsigned propIndex, uint16_t dataRef = 0)
		: Object(const_cast<Object&>(parent), propIndex, dataRef)
	{
	}

	explicit operator bool() const
	{
		return propinfoPtr->type == PropertyType::Object;
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
		return !parent;
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
	 * @note For Union objects this also sets the tag on successful match,
	 * which clears the Object to its default value.
	 */
	Object findObject(const char* name, size_t length);

	Object findObject(const String& name)
	{
		return findObject(name.c_str(), name.length());
	}

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

	Property findProperty(const String& name)
	{
		return findProperty(name.c_str(), name.length());
	}

	/**
	 * @brief Reset contents to defaults (except arrays, which are cleared)
	 * @note Use caution! All reference objects will be invalidated by this call
	 */
	void clear();

	/**
	 * @brief Clear and load all contained arrays with defaults from schema
	 */
	void loadArrayDefaults();

	/**
	 * @brief Does a 'clear' followed by 'loadArrayDefaults'
	 */
	void resetToDefaults();

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

	/**
	 * @brief Support standard streaming output of this object's content in prettified JSON.
	 */
	size_t printTo(Print& p) const;

	/**
	 * @brief Export object to an output stream
	 * @param format Formatter used to generate output
	 * @param output Where to write output
	 * @param options Options for customising output
	 */
	bool exportToStream(const Format& format, Print& output, const ExportOptions& options = {}) const
	{
		return format.exportToStream(*this, output, options);
	}

	/**
	 * @brief Export object to an output stream
	 * @param format Formatter used to generate output
	 * @param filename Where to write output. Non-existent directories are created automatically.
	 * @param options Options for customising output
	 */
	bool exportToFile(const Format& format, const String& filename, const ExportOptions& options = {}) const;

	/**
	 * @brief Import content to this object
	 * @param format Formatter used to read the source data
	 * @param source The source data, not including the name of the object itself
	 */
	Status importFromStream(const Format& format, Stream& source)
	{
		return format.importFromStream(*this, source);
	}

	/**
	 * @brief Import content to this object
	 * @param format Formatter used to read the source data
	 * @param filename File containing source data, not including the name of the object itself
	 */
	Status importFromFile(const Format& format, const String& filename);

	const PropertyInfo& propinfo() const
	{
		return *propinfoPtr;
	}

	const ObjectInfo& typeinfo() const
	{
		return *this ? *propinfoPtr->variant.object : ObjectInfo::empty;
	}

	PropertyData* getPropertyData(unsigned index)
	{
		return PropertyData::fromStruct(typeinfo().getProperty(index), getDataPtr());
	}

	const PropertyData* getPropertyData(unsigned index) const
	{
		return PropertyData::fromStruct(typeinfo().getProperty(index), getDataPtr());
	}

	const PropertyData* getDefaultPropertyData(unsigned index) const;

	/**
	 * @brief Called from `OuterObjectTemplate` methods
	 */
	static void registerCallback(Database& db, uint8_t storeIndex, Callback callback, CallbackType type);

	void getObjectRef(ObjectRef& ref, StoreRef& store) const;
	void getObjectRef(ObjectUpdateRef& ref, StoreUpdateRef& store);

protected:
	friend class Union;
	friend class Accessor;

	StoreRef openStore(Database& db, unsigned storeIndex);
	StoreUpdateRef openStoreForUpdate(Database& db, unsigned storeIndex);

	void disposeArrays();
	void initArrays();

	bool isWriteable() const;

	StoreUpdateRef lockStore(StoreRef& store);

	bool writeCheck() const;

	void* getDataPtr();

	const void* getDataPtr() const;

	String getPropertyString(unsigned index, StringId id) const;

	String getPropertyString(unsigned index) const;

	StringId getStringId(const PropertyInfo& prop, const char* value, uint16_t valueLength);

	StringId getStringId(const PropertyInfo& prop, const String& value)
	{
		return value ? getStringId(prop, value.c_str(), value.length()) : 0;
	}

	template <typename T> StringId getStringId(const PropertyInfo& prop, const T& value)
	{
		return getStringId(prop, toString(value));
	}

	int findStringId(const char* value, uint16_t valueLength) const;

	void resetPropertyValue(unsigned index);
	void setPropertyValue(unsigned index, int64_t value);
	void setPropertyValue(unsigned index, Number value);
	void setPropertyValue(unsigned index, const String& value);

	const PropertyInfo* propinfoPtr;
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
	using Object::Object;
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
 * 				object
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
 * 				item (index)
 * 					parent (offset)
 * 						object
 * 
 * So we need a StoreRef plus four objects.
 */
struct ObjectRef {
	StoreRef store; ///< Keep reference to store so it persists
	Object array;
	Object item;
	Object parent; ///< Either the store or an array owned by the store
	Object object;

	explicit operator bool() const
	{
		return bool(object);
	}
};

struct ObjectUpdateRef {
	StoreUpdateRef store;
	Object array;
	Object item;
	Object parent; ///< Either the store or an array owned by the store
	Object object;

	explicit operator bool() const
	{
		return bool(object);
	}
};

/**
 * @brief Used by code generator
 * @tparam UpdaterType
 * @tparam DatabaseClassType
 * @tparam storeIndex
 * @tparam ParentClassType
 * @tparam propIndex
 * @tparam offset
 */
template <class UpdaterType, class DatabaseClassType, unsigned storeIndex, class ParentClassType, unsigned propIndex,
		  unsigned offset>
class OuterObjectUpdaterTemplate : public UpdaterType
{
public:
	OuterObjectUpdaterTemplate(StoreUpdateRef store)
		: UpdaterType(*store, ParentClassType::typeinfo.getObject(propIndex), offset), store(store)
	{
	}

	explicit OuterObjectUpdaterTemplate(DatabaseClassType& db)
		: OuterObjectUpdaterTemplate(this->openStoreForUpdate(db, storeIndex))
	{
	}

	/**
	 * @brief Create a write-only stream for importing data to this object
	 * @param format Format of the incoming data
	 */
	std::unique_ptr<ImportStream> createImportStream(const Format& format)
	{
		return format.createImportStream(store, *this);
	}

	explicit operator bool() const
	{
		return store && UpdaterType::operator bool();
	}

	StoreUpdateRef store;
};

/**
 * @brief Used by code generator
 * @tparam ContainedClassType
 * @tparam UpdaterType
 * @tparam DatabaseClassType
 * @tparam storeIndex
 * @tparam ParentClassType
 * @tparam propIndex
 * @tparam offset
 */
template <class ContainedClassType, class UpdaterType, class DatabaseClassType, unsigned storeIndex,
		  class ParentClassType, unsigned propIndex, unsigned offset>
class OuterObjectTemplate : public ContainedClassType
{
public:
	using Updater = UpdaterType;

	OuterObjectTemplate(StoreRef store)
		: ContainedClassType(*store, ParentClassType::typeinfo.getObject(propIndex), offset), store(store)
	{
	}

	OuterObjectTemplate(DatabaseClassType& db) : OuterObjectTemplate(this->openStore(db, storeIndex))
	{
	}

	/**
	 * @brief Create a read-only stream for serializing object contents
	 * @param format Formatter used to generate output
	 * @param path JSONPath-like expression to restrict output to specific store or object
	 * @param options Options for customising output
	 */
	std::unique_ptr<ExportStream> createExportStream(const Format& format, const ExportOptions& options = {}) const
	{
		return format.createExportStream(store, *this, options);
	}

	using OuterUpdater =
		OuterObjectUpdaterTemplate<UpdaterType, DatabaseClassType, storeIndex, ParentClassType, propIndex, offset>;

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
		return OuterUpdater(this->lockStore(store));
	}

	using UpdateCallback = Delegate<void(UpdaterType)>;

	static void registerCallback(Database& db, UpdateCallback callback, CallbackType type)
	{
		Object::registerCallback(
			db, storeIndex,
			[callback](Store& store) {
				callback(UpdaterType(store, ParentClassType::typeinfo.getObject(propIndex), offset));
			},
			type);
	}

	/**
	 * @brief Run an update immediately if possible, otherwise queue it
	 * @param callback User callback which will receive an updater instance
	 * @retval bool true if update was performed immediately, false if it's been queued
	 */
	bool update(UpdateCallback callback)
	{
		if(auto upd = update()) {
			callback(upd);
			return true;
		}
		update(this->getDatabase(), std::move(callback));
		return false;
	}

	/**
	 * @brief Run an update asynchronously
	 * @param callback User callback which will receive an updater instance
	 */
	static void update(Database& db, UpdateCallback callback)
	{
		registerCallback(db, std::move(callback), CallbackType::update);
	}

	/**
	 * @brief Register callback just before changes are about to be committed to this object
	 * @param callback User callback which will receive an updater instance
	 */
	void onCommit(UpdateCallback callback)
	{
		onCommit(this->getDatabase(), std::move(callback));
	}

	/**
	 * @brief Register commit callback without instantiating/loading object
	 * @param db Database which manages this object
	 * @param callback User callback which will receive an updater instance
	 */
	static void onCommit(Database& db, UpdateCallback callback)
	{
		registerCallback(db, std::move(callback), CallbackType::commit);
	}

	StoreRef store;
};

} // namespace ConfigDB
