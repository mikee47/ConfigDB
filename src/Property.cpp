#include "include/ConfigDB/Property.h"
#include "include/ConfigDB/Object.h"
#include <Data/Format/Standard.h>

String toString(ConfigDB::Property::Type type)
{
	switch(type) {
#define XX(name)                                                                                                       \
	case ConfigDB::Property::Type::name:                                                                               \
		return F(#name);
		CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
	}
	return nullptr;
}

namespace ConfigDB
{
String Property::getStringValue() const
{
	String value;
	if(name) {
		value = object.getStringValue(*name);
		if(!value && defaultValue) {
			value = *defaultValue;
		}
	}
	return value;
}

String Property::getJsonValue() const
{
	if(!name) {
		return nullptr;
	}
	String value = getStringValue();
	if(!value) {
		return "null";
	}
	switch(type) {
	case Type::Integer:
	case Type::Boolean:
		return value;
	case Type::String:
		break;
	}
	Format::standard.quote(value);
	return value;
}

} // namespace ConfigDB
