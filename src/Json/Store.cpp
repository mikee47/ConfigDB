/**
 * ConfigDB/Json/Store.cpp
 *
 * Copyright 2024 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the ConfigDB Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#include <ConfigDB/Json/Store.h>
#include <ConfigDB/Database.h>
#include <ConfigDB/Pool.h>
#include <Data/Stream/FileStream.h>
#include <Data/Buffer/PrintBuffer.h>
#include <JSON/StreamingParser.h>
#include <Data/Format/Json.h>

namespace
{
String quote(String s)
{
	::Format::json.escape(s);
	::Format::json.quote(s);
	return s;
}
} // namespace

namespace ConfigDB::Json
{
class ConfigListener : public JSON::Listener
{
public:
	using Element = JSON::Element;

	ConfigListener(Store& store) : store(store)
	{
	}

	bool startElement(const Element& element) override
	{
		// debug_i("%s %u %s: '%s' = %u / %s", __FUNCTION__, element.level, toString(element.type).c_str(), element.key,
		// 		element.valueLength, element.value);

		if(element.level == 0) {
			info[0] = Object(store.typeinfo(), store);
			return true;
		}

		auto& parent = info[element.level - 1];
		if(!parent) {
			return true;
		}

		if(element.isContainer()) {
			Object obj;
			if(parent.typeinfo().type == ObjectType::ObjectArray) {
				obj = static_cast<ObjectArray&>(parent).addItem();
			} else {
				obj = parent.findObject(element.key, element.keyLength);
				if(!obj) {
					debug_w("[JSON] Object '%s' not in schema", element.key);
				}
			}
			info[element.level] = obj;
			return true;
		}

		if(parent.typeinfo().type == ObjectType::Array) {
			static_cast<Array&>(parent).addNewItem(element.value, element.valueLength);
			return true;
		}
		auto prop = parent.findProperty(element.key, element.keyLength);
		if(!prop) {
			debug_w("[JSON] Property '%s' not in schema", element.key);
			return true;
		}
		prop.setValueString(element.value, element.valueLength);
		return true;
	}

	bool endElement(const Element&) override
	{
		// Continue parsing
		return true;
	}

private:
	Store& store;
	Object info[JSON::StreamingParser::maxNesting]{};
};

bool Store::load()
{
	auto& root = typeinfo();
	memcpy_P(rootData.get(), root.defaultData, root.structSize);
	stringPool.clear();
	arrayPool.clear();

	String filename = getFilename();
	FileStream stream;
	if(!stream.open(filename, File::ReadOnly)) {
		if(stream.getLastError() != IFS::Error::NotFound) {
			debug_w("open('%s') failed", filename.c_str());
		}
		return true;
	}

	ConfigListener listener(*this);
	JSON::StaticStreamingParser<1024> parser(&listener);
	auto status = parser.parse(stream);

	if(status == JSON::Status::EndOfDocument) {
		return true;
	}

	debug_w("JSON load '%s': %s", filename.c_str(), toString(status).c_str());
	return false;
}

bool Store::save()
{
	String filename = getFilename();
	FileStream stream;
	if(stream.open(filename, File::WriteOnly | File::CreateNewAlways)) {
		StaticPrintBuffer<512> buffer(stream);
		printObjectTo(*this, &fstr_empty, 0, buffer);
	}

	if(stream.getLastError() == FS_OK) {
		debug_d("[JSON] Store saved '%s' OK", filename.c_str());
		return true;
	}

	debug_e("[JSON] Store save '%s' failed: %s", filename.c_str(), stream.getLastErrorString().c_str());
	return false;
}

String Store::getValueJson(const PropertyInfo& info, const void* data) const
{
	String s = getValueString(info, data);
	return s ? (info.type == PropertyType::String) ? quote(s) : s : "null";
}

size_t Store::printObjectTo(const Object& object, const FlashString* name, unsigned nesting, Print& p) const
{
	size_t n{0};

	bool isObject = (object.typeinfo().type <= ObjectType::Object);

	auto pretty = (getDatabase().getFormat() == Format::Pretty);
	auto newline = [&]() {
		if(pretty) {
			n += p.println();
		}
	};

	String indent;
	if(pretty) {
		indent.pad(nesting * 2);
	}
	const char* colon = pretty ? ": " : ":";
	if(name) {
		if(name->length()) {
			if(pretty) {
				n += p.print(indent);
			}
			n += p.print(quote(*name));
			n += p.print(colon);
		}
		n += p.print(isObject ? '{' : '[');
	} else {
		--nesting;
	}
	unsigned itemCount = 0;

	auto objectCount = object.getObjectCount();
	for(unsigned i = 0; i < objectCount; ++i) {
		auto obj = const_cast<Object&>(object).getObject(i);
		if(itemCount++) {
			n += p.print(',');
		}
		newline();
		if(pretty && object.typeinfo().type == ObjectType::ObjectArray) {
			n += p.print(indent);
			n += p.print("  ");
		}
		n += printObjectTo(obj, &obj.typeinfo().name, nesting + 1, p);
	}

	auto propertyCount = object.getPropertyCount();
	for(unsigned i = 0; i < propertyCount; ++i) {
		auto prop = const_cast<Object&>(object).getProperty(i);
		if(itemCount++) {
			n += p.print(',');
		}
		newline();
		if(pretty) {
			n += p.print(indent);
			n += p.print("  ");
		}
		if(prop.typeinfo().name.length()) {
			n += p.print(quote(prop.typeinfo().name));
			n += p.print(colon);
		}
		n += p.print(prop.getJsonValue());
	}

	if(name) {
		if(pretty && itemCount) {
			newline();
			n += p.print(indent);
		}
		n += p.print(isObject ? '}' : ']');
	}

	return n;
}

} // namespace ConfigDB::Json
