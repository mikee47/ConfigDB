/****
 * ConfigDB/JsonRPC/Message.h
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

#pragma once

#include <Print.h>
#include <ConfigDB/Object.h>

namespace JsonRPC
{
#define STRING_MAP(XX)                                                                                                 \
	XX(method)                                                                                                         \
	XX(none)                                                                                                           \
	XX(params)                                                                                                         \
	XX(request)                                                                                                        \
	XX(result)                                                                                                         \
	XX(notification)                                                                                                   \
	XX(error)

#define XX(tag) DECLARE_FSTR(FS_##tag)
STRING_MAP(XX)
#undef XX

/** 
 * @brief RPC metadata
 */
struct Message {
	enum class Kind {
		none,		  ///< No valid message
		request,	  ///< May have params (optional)
		notification, ///< Same as a *request* but with no ID
		result,		  ///< Successful response to a request
		error,		  ///< An error response
	};

	int id{0};
	Kind kind{};
	String method;

	explicit operator bool() const
	{
		return kind != Kind::none;
	}
};

} // namespace JsonRPC

String toString(JsonRPC::Message::Kind kind);
