/****
 * ConfigDB/JsonRPC/WriteStream.cpp
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

#include <ConfigDB/JsonRPC/WriteStream.h>

namespace JsonRPC
{
bool WriteStream::startElement(const Element& element)
{
	if(element.level < 1) {
		return true;
	}

	if(element.level > 1) {
		// We can only fully parse the message once the kind has been established
		if(msg.kind != Message::Kind::none) {
			return ConfigDB::Json::WriteStream::startElement(element);
		}
		return true;
	}

	if(element.keyIs(FS_method)) {
		if(msg.method) {
			return true;
		}
		body = callback.getParamsObject(element.as<String>());
		if(!body) {
			debug_e("[JRPC] Missing %s", element.value);
			return false;
		}
		info[1] = body.object;

		msg.method = element.as<String>();
		return true;
	}

	if(element.keyIs("id")) {
		msg.id = element.as<int>();
		haveId = true;
		return true;
	}

	if(element.keyIs(FS_params)) {
		if(!msg.method) {
			// Cannot decode: we need id to determine request type
			repeatParse = true;
			return true;
		}

		msg.kind = haveId ? Message::Kind::request : Message::Kind::notification;
		return true;
	}

	if(element.keyIs(FS_result)) {
		if(!haveId) {
			// Cannot decode: we need id to determine request type
			repeatParse = true;
			return true;
		}

		body = callback.getResultObject(msg.id);
		if(!body) {
			debug_e("[JRPC] Unknown ID %d", msg.id);
			return false;
		}
		info[1] = body.object;

		msg.kind = Message::Kind::result;
		return true;
	}

	if(element.keyIs(FS_error)) {
		if(!haveId) {
			// Cannot decode: we need id to determine request type
			repeatParse = true;
			return true;
		}

		body = callback.getErrorObject(msg.id);
		if(!body) {
			debug_e("[JRPC] Missing %s", element.key);
			return false;
		}
		info[1] = body.object;

		msg.kind = Message::Kind::error;
		return true;
	}

	return true;
}

} // namespace JsonRPC
