/**
 * ConfigDB/Format.cpp
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

#include "include/ConfigDB/Format.h"
#include <Data/Stream/FileStream.h>
#include <Data/Buffer/PrintBuffer.h>

namespace ConfigDB
{
bool Format::exportToFile(const Store& store, const String& filename) const
{
	FileStream stream;
	if(stream.open(filename, File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<512> buffer(stream);
		exportToStream(store, buffer);
	}

	if(stream.getLastError() == FS_OK) {
		debug_d("[JSON] Store saved '%s' OK", filename.c_str());
		return true;
	}

	debug_e("[JSON] Store save '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
	return false;
}

bool Format::exportToFile(const Store& store) const
{
	String filename = store.getFilePath() + getFileExtension();
	return exportToFile(store, filename);
}

bool Format::exportToFile(Database& database, const String& filename) const
{
	FileStream stream;
	if(stream.open(filename, File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<512> buffer(stream);
		exportToStream(database, buffer);
	}

	if(stream.getLastError() == FS_OK) {
		debug_d("[JSON] Store saved '%s' OK", filename.c_str());
		return true;
	}

	debug_e("[JSON] Store save '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
	return false;
}

bool Format::importFromFile(Store& store, const String& filename) const
{
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		if(stream.getLastError() == IFS::Error::NotFound) {
			return true;
		}
		debug_w("open('%s') failed", filename.c_str());
		return false;
	}

	return importFromStream(store, stream);
}

bool Format::importFromFile(Store& store) const
{
	String filename = store.getFilePath() + getFileExtension();
	return importFromFile(store, filename);
}

bool Format::importFromFile(Database& database, const String& filename) const
{
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		debug_w("open('%s') failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
		return false;
	}

	return importFromStream(database, stream);
}

} // namespace ConfigDB
