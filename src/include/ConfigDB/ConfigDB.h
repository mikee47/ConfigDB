#pragma once

#include <WString.h>
#include <Data/CString.h>

namespace ConfigDB
{
enum class Mode {
	readonly,
	readwrite,
};

class Database
{
public:
	/**
	 * @brief Database instance
	 * @param path Path to root directory where all data is stored
	 * @param mode Required access mode
	 */
	Database(const String& path, Mode mode) : path(path.c_str()), mode(mode)
	{
	}

	String getPath() const
	{
		return path.c_str();
	}

	Mode getMode() const
	{
		return mode;
	}

private:
	CString path;
	Mode mode;
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
	}

	/**
	 * @brief Commit changes, must fail in readonly mode
	 */
	bool commit()
	{
		return false;
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
		path += name;
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

private:
	Database& db;
	String name;
};

class Group
{
public:
	Group(const String& path) : path(path)
	{
	}

	String getPath() const
	{
		return path;
	}

protected:
	String path;
};

} // namespace ConfigDB
