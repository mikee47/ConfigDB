#include "include/ConfigDB/Json.h"
#include <Data/CStringArray.h>
#include <Data/Stream/MemoryDataStream.h>

namespace ConfigDB::Json
{
JsonObject Store::getJsonObject(const String& path)
{
	// debug_i("getObject(%s)", path.c_str());
	String s(path);
	s.replace('.', '\0');
	CStringArray csa(std::move(s));
	auto obj = getRootJsonObject();
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

JsonObject Store::getRootJsonObject()
{
	return doc.isNull() ? doc.to<JsonObject>() : doc.as<JsonObject>();
}

std::unique_ptr<IDataSourceStream> Store::serialize() const
{
	auto stream = std::make_unique<MemoryDataStream>();
	if(stream) {
		::Json::serialize(doc, stream.get());
	}
	return stream;
}

size_t Object::printTo(Print& p) const
{
	auto obj = store->getJsonObject(getName());
	return ::Json::serialize(obj, p);
}

} // namespace ConfigDB::Json
