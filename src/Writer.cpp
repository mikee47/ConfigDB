/**
 * ConfigDB/Writer.cpp
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

#include "include/ConfigDB/Writer.h"
#include <Data/Stream/FileStream.h>

namespace ConfigDB
{
bool Writer::loadFromFile(Store& store, const String& filename)
{
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		if(stream.getLastError() == IFS::Error::NotFound) {
			return true;
		}
		debug_w("open('%s') failed", filename.c_str());
		return false;
	}

	return loadFromStream(store, stream);
}

bool Writer::loadFromFile(Store& store)
{
	String filename = store.getFilePath() + getFileExtension();
	return loadFromFile(store, filename);
}

bool Writer::loadFromFile(Database& database, const String& filename)
{
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		debug_w("open('%s') failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
		return false;
	}

	return loadFromStream(database, stream);
}

} // namespace ConfigDB
