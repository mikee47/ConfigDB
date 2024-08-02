#include <ConfigDB/Json/Object.h>

class Connection : public ConfigDB::Json::ObjectTemplate<Connection>
{
public:
	class NetworkRef : public ConfigDB::Json::Object
	{
	public:
		ConfigDB::Json::Store parent;
	};
};
