#include <Print.h>
#include <ConfigDB/Object.h>

namespace JsonRPC
{
/** 
 * @brief RPC metadata
 */
struct Message {
	enum class Kind {
		none,		  ///< No valid message
		request,	  ///< May have params (optional)
		notification, ///< Same as a result but with no ID
		result,		  ///< Successful response to a request
		error,		  ///< An error response
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

String toString(JsonRPC::Message::Kind kind);
