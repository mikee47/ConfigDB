#pragma once

#include "ConfigDB.h"
#include <ArduinoJson.h>

namespace ConfigDB::Json
{
class Store : public ConfigDB::Store
{
public:
	Store(Database& db, const String& name) : ConfigDB::Store(db, name), doc(1024)
	{
		::Json::loadFromFile(doc, getFilename());
	}

	bool commit()
	{
		if(database().getMode() != Mode::readwrite) {
			return false;
		}
		return ::Json::saveToFile(doc, getFilename(), ::Json::Pretty);
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

	template <typename T> bool setValue(const String& key, const T& value)
	{
		return setValue(nullptr, key, value);
	}

	template <typename T> T getValue(const String& path, const String& key)
	{
		return getObject(path)[key];
	}

	template <typename T> T getValue(const String& key)
	{
		return getRootObject()[key];
	}

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
	Object(Store& store, const String& path) : ConfigDB::Object(path), store(store)
	{
	}

	template <typename T> bool setValue(const String& key, const T& value)
	{
		return store.setValue(path, key, value);
	}

	template <typename T> T getValue(const String& key)
	{
		return store.getValue<T>(path, key);
	}

protected:
	Store& store;
};

} // namespace ConfigDB::Json
