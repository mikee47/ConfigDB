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

	struct Content {
		const char* start;
		unsigned length;

		explicit operator bool() const
		{
			return start && length != 0;
		}
	};

	int id{0};
	String method;
	Kind kind{};
	Content content{};
	bool isContainer{false};
};

Message importMessage(ConfigDB::Database& db, const String& jsonString);

bool exportMessage(ConfigDB::Database& db, int id, Print& out);

} // namespace JsonRPC
