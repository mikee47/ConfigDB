#pragma once

#include "ConfigDB.h"
#include <ArduinoJson.h>

namespace ConfigDB::Json
{
class Store : public ConfigDB::Store
{
public:
	Store(const String& name, Mode mode) : doc(1024), filename(name + ".json"), mode(mode)
	{
		::Json::loadFromFile(doc, filename);
	}

	bool commit()
	{
		if(mode != Mode::readwrite) {
			return false;
		}
		return ::Json::saveToFile(doc, filename, ::Json::Pretty);
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

	template <typename T> T getValue(const String& path, const String& key)
	{
		JsonObject obj = getObject(path);
		return obj[key];
	}

protected:
	/**
	 * @brief Resolve a path into the corresponding JSON object, creating it if required
	 */
	JsonObject getObject(const String& path);

private:
	DynamicJsonDocument doc;
	String filename;
	Mode mode;
};

/**
 * @brief Access a group of simple key/value pairs within a store
 */
class Group
{
public:
	Group(Store& store, const String& path) : store(store), path(path)
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

private:
	Store& store;
	String path;
};

} // namespace ConfigDB::Json
