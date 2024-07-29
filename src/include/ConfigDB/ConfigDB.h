#pragma once

#include <WString.h>
#include <Data/CString.h>
#include <Data/Stream/DataSourceStream.h>
#include <debug_progmem.h>

namespace ConfigDB
{
enum class Mode {
	readonly,
	readwrite,
};

class Store;
class Object;

class Database
{
public:
	virtual ~Database()
	{
	}

	/**
	 * @brief Database instance
	 * @param path Path to root directory where all data is stored
	 */
	Database(const String& path) : path(path.c_str())
	{
	}

	String getPath() const
	{
		return path.c_str();
	}

	/**
	 * @brief Get stores
	 */
	virtual std::shared_ptr<Store> getStore(unsigned index) = 0;

private:
	CString path;
};

/**
 * @brief Non-virtual implementation template
 */
class Store
{
public:
	/**
	 * @brief Storage instance
	 * @param db Database to which this store belongs
	 * @param name Name of store, used as key in JSONPath
	 */
	Store(Database& db, const String& name) : db(db), name(name)
	{
		debug_d("%s(%s)", __FUNCTION__, name.c_str());
	}

	virtual ~Store()
	{
		debug_d("%s(%s)", __FUNCTION__, name.c_str());
	}

	/**
	 * @brief Commit changes
	 */
	virtual bool commit() = 0;

	bool isRoot() const
	{
		return name.length() == 0;
	}

	/**
	 * @brief Store a value
	 * @param path JSONPath object location
	 * @param key Key for value
	 * @param value Value to store
	 * @retval bool true on success
	 */
	template <typename T> bool setValue(const String& path, const String& key, const T& value)
	{
		return false;
	}

	/**
	 * @brief Retrieve a value
	 * @param path JSONPath object location
	 * @param key Key for value
	 * @param value Variable to store result, undefined on failure
	 * @retval bool true on success
	 */
	template <typename T> bool getValue(const String& path, const String& key, T& value)
	{
		return false;
	}

	String getPath() const
	{
		String path = db.getPath();
		path += '/';
		path += name ?: F("_root");
		return path;
	}

	Database& database()
	{
		return db;
	}

	const String& getName() const
	{
		return name;
	}

	virtual std::unique_ptr<IDataSourceStream> serialize() const = 0;

	/**
	 * @brief Get top-level objects
	 */
	virtual std::unique_ptr<Object> getObject(unsigned index) = 0;

private:
	Database& db;
	String name;
};

template <class BaseType, class ClassType> class StoreTemplate : public BaseType
{
public:
	using BaseType::BaseType;

	static std::shared_ptr<ClassType> open(Database& db)
	{
		auto inst = store.lock();
		if(!inst) {
			inst = std::make_shared<ClassType>(db);
			store = inst;
		}
		return inst;
	}

	static std::shared_ptr<ClassType> getPointer()
	{
		return store.lock();
	}

private:
	static std::weak_ptr<ClassType> store;
};

template <class BaseType, class ClassType> std::weak_ptr<ClassType> StoreTemplate<BaseType, ClassType>::store;

class Object
{
public:
	Object(const String& name) : name(name)
	{
	}

	virtual ~Object()
	{
	}

	const String& getName() const
	{
		return name;
	}

	String getPath() const
	{
		String path = getStore()->getName();
		if(path && name) {
			path += '.';
			path += name;
		}
		return path;
	}

	virtual std::shared_ptr<Store> getStore() const = 0;

	/**
	 * @brief Commit changes to the store
	 */
	bool commit()
	{
		return getStore()->commit();
	}

	virtual size_t printTo(Print& p) const = 0;

private:
	String name;
};

} // namespace ConfigDB
