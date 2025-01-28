#pragma once

#include <SmingTest.h>
#include <test-config.h>
#include <Data/Stream/MemoryDataStream.h>
#include <ConfigDB/Json/Format.h>

extern TestConfig database;

void resetDatabase();

template <typename T> bool importObject(T& object, const String& data)
{
	MemoryDataStream mem;
	mem << data;
	auto status = object.importFromStream(ConfigDB::Json::format, mem);
	return bool(status);
}

inline bool importObject(ConfigDB::Database& db, const String& data)
{
	MemoryDataStream mem;
	mem << data;
	auto status = db.importFromStream(ConfigDB::Json::format, mem);
	return bool(status);
}

template <typename T> String exportObject(T& object)
{
	MemoryDataStream mem;
	object.exportToStream(ConfigDB::Json::format, mem);
	String s;
	mem.moveString(s);
	return s;
}
