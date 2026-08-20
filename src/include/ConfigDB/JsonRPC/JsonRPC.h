#pragma once

#include <Print.h>
#include <ConfigDB/Database.h>
#include "WriteStream.h"

namespace JsonRPC
{

/**
 * @brief Decode a JSON-RPC message into the database's ConfigDB working store.
 * @param db
 * @param jsonString
 * @param getRequestTag Callback invoked when importing responses
 */
Message importMessage(ConfigDB::Database& db, const String& jsonString, GetRequestTag getRequestTag);

/**
 * @brief Create a message in JSON-RPC 2.0 format
 * @param db Contains payload for message
 * @param msg Message metadata
 * @param out Where to write message
 * @retval bool False if no message kind is selected or the store cannot be opened
 */
bool exportMessage(ConfigDB::Database& db, const Message& msg, Print& out);

} // namespace JsonRPC
