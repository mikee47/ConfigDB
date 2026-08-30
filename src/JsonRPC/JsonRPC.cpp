/****
 * ConfigDB/JsonRPC/JsonRPC.cpp
 *
 * Copyright 2026 mikee47 <mike@sillyhouse.net>
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

#include <ConfigDB/JsonRPC/JsonRPC.h>
#include <ConfigDB/Database.h>
#include <ConfigDB/JsonRPC/ReadStream.h>
#include <ConfigDB/JsonRPC/WriteStream.h>

namespace JsonRPC
{
Message importMessage(ConfigDB::Database& db, const String& jsonString, GetRequestTag getRequestTag)
{
	auto store = db.openStoreForUpdate(0);
	if(!store) {
		return {};
	}
	store->resetToDefaults();

	Message msg{};
	WriteStream stream(*store, msg, getRequestTag);
	stream.print(jsonString);

	if(stream.isReparseRequired()) {
		// Second pass
		stream.reset();
		stream.print(jsonString);
	}

	auto status = stream.getStatus();

	if(!status) {
		debug_e("importMessage failed: %s", toString(status).c_str());
		return {};
	}

	return msg;
}

bool exportMessage(ConfigDB::Database& db, const Message& msg, Print& out)
{
	return JsonRPC::ReadStream::print(db, msg, out) != 0;
}

} // namespace JsonRPC
