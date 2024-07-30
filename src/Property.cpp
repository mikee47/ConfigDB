#include "include/ConfigDB/Property.h"
#include "include/ConfigDB/Object.h"

namespace ConfigDB
{
String Property::getStringValue() const
{
	return object.getStringValue(name);
}

String Property::getJsonValue() const
{
	switch(type) {
	case Type::Invalid:
		return nullptr;
	case Type::Null:
		return "null";
	case Type::Integer:
		return getStringValue();
	case Type::Boolean:
		return getStringValue().toInt() ? "True" : "False";
	case Type::String:
		break;
	}
	String value = getStringValue();
	String s;
	unsigned quoteCount = 0;
	for(auto c : value) {
		if(c == '"') {
			++quoteCount;
		}
	}
	s.setLength(value.length() + 2 + quoteCount);
	auto ptr = s.begin();
	for(auto c : value) {
		if(c == '"') {
			*ptr++ = '\\';
		}
		*ptr++ = c;
	}
	*ptr = '"';
	return s;
}

} // namespace ConfigDB
