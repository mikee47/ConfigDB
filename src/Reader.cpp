/**
 * ConfigDB/Reader.cpp
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

#include "include/ConfigDB/Reader.h"
#include <Data/Stream/FileStream.h>
#include <Data/Buffer/PrintBuffer.h>

namespace ConfigDB
{
bool Reader::saveToFile(const Store& store, const String& filename)
{
	FileStream stream;
	if(stream.open(filename, File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<512> buffer(stream);
		printObjectTo(store, &fstr_empty, 0, buffer);
	}

	if(stream.getLastError() == FS_OK) {
		debug_d("[JSON] Store saved '%s' OK", filename.c_str());
		return true;
	}

	debug_e("[JSON] Store save '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
	return false;
}

bool Reader::saveToFile(const Store& store)
{
	String filename = store.getFilePath() + getFileExtension();
	return saveToFile(store, filename);
}

} // namespace ConfigDB
