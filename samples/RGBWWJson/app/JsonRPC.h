#include <Print.h>
#include <ConfigDB/Object.h>

namespace JsonRPC
{
/** Describes which JSON-RPC payload was decoded and its correlation ID. */
struct Message {
	enum class Kind {
		none,
		params,
		result,
		error,
	};

	int id{0};
	Kind kind{};
};

/**
 * @brief Applications should implement this method
 * @param id Received message RPC ID
 * @retval int Root tag index for corresponding request, -1 if unknown
 */
using GetRequestTag = Delegate<int(int id)>;

/**
 * Decode a JSON-RPC message into the database's ConfigDB working store.
 *
 * Requests select a schema using their method name. Responses use
 * getRequestTag to recover the schema associated with their request ID.
 * An empty Message (kind == none) indicates that parsing failed.
 */
Message importMessage(ConfigDB::Database& db, const String& jsonString, GetRequestTag getRequestTag);

/**
 * Write the selected ConfigDB payload as a JSON-RPC 2.0 message.
 * Returns false when no message kind is selected or the store cannot be opened.
 */
bool exportMessage(ConfigDB::Database& db, const Message& msg, Print& out);

} // namespace JsonRPC

String toString(JsonRPC::Message::Kind kind);
