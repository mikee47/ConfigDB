/****
 * ConfigDB/JsonRPC/Message.cpp
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

#include <ConfigDB/JsonRPC/Message.h>

namespace JsonRPC
{
#define XX(tag) DEFINE_FSTR(FS_##tag, #tag)
STRING_MAP(XX)
#undef XX

} // namespace JsonRPC

String toString(JsonRPC::Message::Kind kind)
{
	using namespace JsonRPC;
	using Kind = Message::Kind;
	switch(kind) {
	case Kind::none:
		return FS_none;
	case Kind::request:
		return FS_request;
	case Kind::notification:
		return FS_notification;
	case Kind::result:
		return FS_result;
	case Kind::error:
		return FS_error;
	}
	return nullptr;
}
