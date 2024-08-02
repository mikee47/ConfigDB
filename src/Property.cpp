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
	if(!object) {
		return nullptr;
	}

	String value;
	if(name) {
		value = object->getStringValue(*name);
	} else {
		value = object->getStringValue(index);
	}
	if(!value && defaultValue) {
		value = *defaultValue;
	}
	return value;
}

std::unique_ptr<Object> Property::getObjectValue() const
{
	if(!object) {
		return nullptr;
	}
	if(name) {
		return object->getObject(*name);
	}
	return object->getObject(index);
}

String Property::getJsonValue() const
{
	if(!object) {
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
	case Type::Object:
	case Type::Array:
		return value;
	}
	::Format::standard.quote(value);
	return value;
}

} // namespace ConfigDB
