#include <Print.h>
#include <ConfigDB/Object.h>

namespace JsonRPC
{
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

Message importMessage(ConfigDB::Database& db, const String& jsonString, GetRequestTag getRequestTag);

bool exportMessage(ConfigDB::Database& db, const Message& msg, Print& out);

} // namespace JsonRPC

String toString(JsonRPC::Message::Kind kind);
