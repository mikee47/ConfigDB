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
		return ::Json::saveToFile(doc, filename);
	}

	template <typename T> bool setValue(const String& key, const T& value)
	{
		return false;
	}

	template <typename T> T getValue(const String& key)
	{
		return T{};
	}

protected:
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
	Group(Store& store, const String& path) : store(store)
	{
	}

	template <typename T> bool setValue(const String& key, const T& value)
	{
		return store.setValue(path + key, value);
	}

	template <typename T> T getValue(const String& key)
	{
		return store.getValue<T>(path + key);
	}

private:
	Store& store;
	String path;
};

} // namespace ConfigDB::Json
