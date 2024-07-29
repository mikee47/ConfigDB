#pragma once

#include "ConfigDB.h"
#include <ArduinoJson.h>

namespace ConfigDB::Json
{
class Store : public ConfigDB::Store
{
public:
	using Pointer = std::shared_ptr<Store>;

	/**
	 * @brief Construct a Store accessor
	 * @param db
	 * @param name JsonPATH name for store relative to database root
	 */
	Store(Database& db, const String& name) : ConfigDB::Store(db, name), doc(1024)
	{
		::Json::loadFromFile(doc, getFilename());
	}

	bool commit() override
	{
		return ::Json::saveToFile(doc, getFilename());
	}

	template <typename T> bool setValue(const String& path, const String& key, const T& value)
	{
		JsonObject obj = getObject(path);
		if(!obj) {
			return false;
		}
		obj[key] = value;
		return true;
	}

	template <typename T> T getValue(const String& path, const String& key, const T& defaultValue = {})
	{
		return getObject(path)[key] | defaultValue;
	}

	std::unique_ptr<IDataSourceStream> serialize() const override;

protected:
	String getFilename() const
	{
		return getPath() + ".json";
	}

	/**
	 * @brief Resolve a path into the corresponding JSON object, creating it if required
	 */
	JsonObject getObject(const String& path);

	JsonObject getRootObject();

private:
	DynamicJsonDocument doc;
};

/**
 * @brief Access a Object of simple key/value pairs within a store
 */
class Object : public ConfigDB::Object
{
public:
	Object(Store::Pointer store, const String& path) : ConfigDB::Object(path), store(store)
	{
	}

	template <typename T> bool setValue(const String& key, const T& value)
	{
		return store->setValue(getName(), key, value);
	}

	template <typename T> T getValue(const String& key)
	{
		return store->getValue<T>(getName(), key);
	}

	ConfigDB::Store::Pointer getStore() const override
	{
		return store;
	}

protected:
	Store::Pointer store;
};

} // namespace ConfigDB::Json
