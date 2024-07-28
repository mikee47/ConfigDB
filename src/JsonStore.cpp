#include "include/ConfigDB/Json.h"
#include <Data/CStringArray.h>

namespace ConfigDB::Json
{
JsonObject Store::getObject(const String& path)
{
	// debug_i("getObject(%s)", path.c_str());
	String s(path);
	s.replace('.', '\0');
	CStringArray csa(std::move(s));
	JsonObject obj = doc.isNull() ? doc.to<JsonObject>() : doc.as<JsonObject>();
	for(auto key : csa) {
		if(!obj) {
			break;
		}
		auto child = obj[key];
		if(!child.is<JsonObject>()) {
			child = obj.createNestedObject(const_cast<char*>(key));
		}
		obj = child;
	}
	return obj;
}

} // namespace ConfigDB::Json
