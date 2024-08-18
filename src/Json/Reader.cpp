/**
 * ConfigDB/Json/Reader.cpp
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

#include <ConfigDB/Json/Reader.h>
#include <ConfigDB/Json/DataStream.h>
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
Reader reader;

DEFINE_FSTR(fileExtension, ".json")

size_t Reader::printObjectTo(const Object& object, const FlashString* name, unsigned nesting, Print& p) const
{
	size_t n{0};

	bool isObject = (object.typeinfo().type <= ObjectType::Object);

	auto pretty = (format == Format::Pretty);
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

std::unique_ptr<IDataSourceStream> Reader::createStream(Database& db) const
{
	return std::make_unique<DataStream>(db, db.getFormat());
}

std::unique_ptr<IDataSourceStream> Reader::createStream(std::shared_ptr<Store> store) const
{
	return std::make_unique<DataStream>(store, store->getDatabase().getFormat());
}

} // namespace ConfigDB::Json
