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

#include "Printer.h"
#include <Data/Format/Json.h>

namespace ConfigDB::Json
{
Printer::Printer(Print& p, const Object& object, bool pretty, RootStyle style) : p(&p), pretty(pretty)
{
	objects[0] = object;
	setRootStyle(style, nullptr);
}

void Printer::setRootStyle(RootStyle style, const String& name)
{
	rootStyle = style;
	switch(rootStyle) {
	case RootStyle::hidden:
		rootName = nullptr;
		break;
	case RootStyle::braces:
		rootName = String::empty;
		break;
	case RootStyle::name:
	case RootStyle::object:
		rootName = name ?: String(objects[0].propinfo().name);
		break;
	}

	reset();
}

void Printer::reset()
{
	nesting = 0;
	objects[0].streamPos = 0;
}

size_t Printer::operator()()
{
	if(!p || isDone()) {
		return 0;
	}

	size_t n{0};

	auto& object = objects[nesting];
	auto indentLength = nesting;

	String name;
	if(nesting == 0) {
		name = rootName;
	} else {
		name = object.propinfo().name;
	}
	if(rootName && rootName.length()) {
		++indentLength;
	}

	bool isArray = object.isArray();

	auto quote = [](String s) {
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
		if(name.length()) {
			if(nesting == 0 && rootStyle == RootStyle::object) {
				n += p->print('{');
			}
			if(pretty) {
				n += p->print(indent);
			}
			n += p->print(quote(name));
			n += p->print(colon);
		}
		n += p->print(isArray ? '[' : '{');
	}

	// Print child object
	auto objectCount = object.getObjectCount();
	if(index < objectCount) {
		auto obj = object.getObject(index);
		if(object.streamPos++) {
			n += p->print(',');
		}
		n += newline();
		if(pretty && object.typeIs(ObjectType::ObjectArray)) {
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
		auto prop = static_cast<const Object&>(object).getProperty(index);
		if(object.streamPos++) {
			n += p->print(',');
		}
		n += newline();
		if(pretty) {
			n += p->print(indent);
			n += p->print("  ");
		}
		if(prop.info().name.length()) {
			n += p->print(quote(prop.info().name));
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
		n += p->print(isArray ? ']' : '}');

		if(nesting == 0 && rootStyle == RootStyle::object && name.length()) {
			n += p->print('}');
		}
	}

	--nesting;

	return n;
}

} // namespace ConfigDB::Json
