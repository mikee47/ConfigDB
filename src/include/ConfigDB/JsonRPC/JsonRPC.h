/****
 * ConfigDB/JsonRPC/JsonRPC.h
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
#include <ConfigDB/Database.h>
#include "WriteStream.h"

namespace JsonRPC
{
/**
 * @brief Decode a JSON-RPC message into the database's ConfigDB working store.
 * @param jsonString
 * @param callback Callback invoked when importing responses
 */
Message importMessage(const String& jsonString, WriteStream::Callback& callback);

/**
 * @brief Create a message in JSON-RPC 2.0 format
 * @param msg Message metadata
 * @param body Contains payload for message
 * @param out Where to write message
 * @retval bool False if no message kind is selected or the store cannot be opened
 */
bool exportMessage(const Message& msg, const ConfigDB::Object& body, Print& out);

} // namespace JsonRPC
