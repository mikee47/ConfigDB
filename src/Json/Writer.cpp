/**
 * ConfigDB/Json/Writer.cpp
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

#include <ConfigDB/Json/Writer.h>
#include <ConfigDB/Json/WriteStream.h>
#include <debug_progmem.h>

namespace ConfigDB::Json
{
Writer writer;

bool Writer::loadFromStream(Store& store, Stream& source)
{
	store.clear();
	auto status = WriteStream::parse(store, source);

	if(status == JSON::Status::EndOfDocument) {
		return true;
	}

	debug_w("JSON load '%s': %s", store.getName().c_str(), toString(status).c_str());
	return false;
}

std::unique_ptr<ReadWriteStream> Writer::createStream(Database& db) const
{
	return std::make_unique<WriteStream>(db);
}

std::unique_ptr<ReadWriteStream> Writer::createStream(std::shared_ptr<Store> store) const
{
	return std::make_unique<WriteStream>(store);
}

} // namespace ConfigDB::Json
