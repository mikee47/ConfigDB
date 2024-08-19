/**
 * ConfigDB/Json/Printer.cpp
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

#include <ConfigDB/Json/Printer.h>
#include <Data/Format/Json.h>

namespace ConfigDB::Json
{
// Return true if more data to output
size_t Printer::operator()()
{
	if(!p || isDone()) {
		return 0;
	}

	size_t n{0};

	auto& object = objects[nesting];
	auto name = &object.typeinfo().name;
	auto indentLength = nesting;

	if(nesting == 0) {
		name = rootName;
	}
	if(rootName && rootName->length()) {
		++indentLength;
	}

	bool isObject = (object.typeinfo().type <= ObjectType::Object);

	auto quote = [](String s) {
		::Format::json.escape(s);
		::Format::json.quote(s);
		return s;
	};

	String indent;
	if(pretty) {
		indent.pad(indentLength * 2);
	}
	const char* colon = pretty ? ": " : ":";

	unsigned index = object.streamPos;

	// If required, print object name and opening brace
	if(index == 0 && name) {
		if(name->length()) {
			if(pretty) {
				n += p->print(indent);
			}
			n += p->print(quote(*name));
			n += p->print(colon);
		}
		n += p->print(isObject ? '{' : '[');
	}

	// Print child object
	auto objectCount = object.getObjectCount();
	if(index < objectCount) {
		auto obj = const_cast<Object&>(object).getObject(index);
		if(object.streamPos++) {
			n += p->print(',');
		}
		n += newline();
		if(pretty && object.typeinfo().type == ObjectType::ObjectArray) {
			n += p->print(indent);
			n += p->print("  ");
		}
		++nesting;
		objects[nesting] = obj;
		return n;
	}

	index -= objectCount;

	// Print property
	if(index < object.getPropertyCount()) {
		auto prop = const_cast<Object&>(object).getProperty(index);
		if(object.streamPos++) {
			n += p->print(',');
		}
		n += newline();
		if(pretty) {
			n += p->print(indent);
			n += p->print("  ");
		}
		if(prop.typeinfo().name.length()) {
			n += p->print(quote(prop.typeinfo().name));
			n += p->print(colon);
		}
		n += p->print(prop.getJsonValue());
		return n;
	}

	// If required, print closing brace
	if(name) {
		if(pretty && object.streamPos) {
			n += newline();
			n += p->print(indent);
		}
		n += p->print(isObject ? '}' : ']');
	}

	--nesting;

	return n;
}

} // namespace ConfigDB::Json
