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
			return WriteStream::startElement(element);
		}
		return true;
	}

	if(element.keyIs(FS_method)) {
		auto& request = info[0];
		request = root.findObject(element.value, element.valueLength);
		if(!request) {
			debug_w("[JRPC] Missing %s", element.value);
			return false;
		}

		auto& params = info[1];
		String tag(FS_params);
		params = request.findObject(tag.c_str(), tag.length());
		if(!params) {
			debug_e("[JRPC] Missing %s/%s", element.value, tag.c_str());
			return false;
		}

		haveMethod = true;
		return true;
	}

	if(element.keyIs("id")) {
		msg.id = element.as<int>();
		haveId = true;
		return true;
	}

	if(element.keyIs(FS_params)) {
		if(!haveMethod) {
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

		int tag = getRequestTag(msg.id);
		if(tag < 0) {
			debug_e("[JRPC] Unknown ID %d", msg.id);
			return false;
		}

		root.setTag(tag);
		auto& request = info[0];
		request = root.getObject(0);

		// Result could be a simple value, or an object
		msg.kind = Message::Kind::result;
		if(element.isContainer()) {
			auto& result = info[1];
			result = request.findObject(element.key, element.keyLength);
			if(!result) {
				debug_e("[JRPC] Missing %s/%s", root.getTagString().c_str(), element.key);
				return false;
			}
			return true;
		} else {
			auto prop = root.findProperty(element.key, element.keyLength);
			if(prop && prop.setJsonValue(element.value, element.valueLength)) {
				return true;
			}
		}
		debug_e("[JRPC] Missing result");
		return false;
	}

	if(element.keyIs(FS_error)) {
		if(!haveId) {
			// Cannot decode: we need id to determine request type
			repeatParse = true;
			return true;
		}

		int tag = getRequestTag(msg.id);
		if(tag < 0) {
			debug_e("[JRPC] Unknown ID  %d", msg.id);
			return false;
		}

		root.setTag(tag);
		auto& request = info[0];
		request = root.getObject(0);

		msg.kind = Message::Kind::error;
		auto& error = info[1];
		error = request.findObject(element.key, element.keyLength);
		if(!error) {
			debug_e("[JRPC] Missing %s/%s", root.getTagString().c_str(), element.key);
			return false;
		}
	}

	return true;
}

} // namespace JsonRPC
