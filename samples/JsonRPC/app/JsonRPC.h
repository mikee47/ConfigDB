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
	String method;
	Kind kind{};
};

Message importMessage(ConfigDB::Database& db, const String& jsonString);

bool exportMessage(ConfigDB::Database& db, int id, Print& out);

} // namespace JsonRPC
