#pragma once

#include "ConfigDB.h"
#include <ArduinoJson.h>

namespace ConfigDB::Json
{
class Store : public ConfigDB::Store
{
public:
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
		JsonObject obj = getJsonObject(path);
		if(!obj) {
			return false;
		}
		obj[key] = value;
		return true;
	}

	template <typename T> T getValue(const String& path, const String& key, const T& defaultValue = {})
	{
		return getJsonObject(path)[key] | defaultValue;
	}

	std::unique_ptr<IDataSourceStream> serialize() const override;

	String getFilename() const
	{
		return getPath() + ".json";
	}

	/**
	 * @brief Resolve a path into the corresponding JSON object, creating it if required
	 */
	JsonObject getJsonObject(const String& path);

	JsonObject getRootJsonObject();

private:
	DynamicJsonDocument doc;
};

/**
 * @brief Access a Object of simple key/value pairs within a store
 */
class Object : public ConfigDB::Object
{
public:
	Object(std::shared_ptr<Store> store, const String& path) : ConfigDB::Object(path), store(store)
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

	std::shared_ptr<ConfigDB::Store> getStore() const override
	{
		return store;
	}

	size_t printTo(Print& p) const override;

protected:
	std::shared_ptr<Store> store;
};

} // namespace ConfigDB::Json
