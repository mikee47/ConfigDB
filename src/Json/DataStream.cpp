/**
 * ConfigDB/Json/DataStream.cpp
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

#include <ConfigDB/Json/DataStream.h>
#include <ConfigDB/Json/Reader.h>
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
// Return true if more data to output
void DataStream::printObject(Print& p)
{
	assert(nesting < JSON::StreamingParser::maxNesting);
	auto& object = objects[nesting];
	auto name = &object.typeinfo().name;

	// When streaming entire database, don't print name or braces for root object
	if(nesting == 0 && db && storeIndex == 0) {
		name = nullptr;
	}

	bool isObject = (object.typeinfo().type <= ObjectType::Object);

	auto newline = [&]() {
		if(pretty) {
			p.println();
		}
	};

	String indent;
	if(pretty) {
		indent.pad((nesting + (storeIndex > 0)) * 2);
	}
	const char* colon = pretty ? ": " : ":";

	unsigned index = object.streamPos;

	// If required, print object name and opening brace
	if(index == 0 && name) {
		if(name->length()) {
			if(pretty) {
				p.print(indent);
			}
			p.print(quote(*name));
			p.print(colon);
		}
		p.print(isObject ? '{' : '[');
	}

	// Print child object
	auto objectCount = object.getObjectCount();
	if(index < objectCount) {
		auto obj = const_cast<Object&>(object).getObject(index);
		if(object.streamPos++) {
			p.print(',');
		}
		newline();
		if(pretty && object.typeinfo().type == ObjectType::ObjectArray) {
			p.print(indent);
			p.print("  ");
		}
		++nesting;
		objects[nesting] = obj;
		return;
	}

	index -= objectCount;

	// Print property
	if(index < object.getPropertyCount()) {
		auto prop = const_cast<Object&>(object).getProperty(index);
		if(object.streamPos++) {
			p.print(',');
		}
		newline();
		if(pretty) {
			p.print(indent);
			p.print("  ");
		}
		if(prop.typeinfo().name.length()) {
			p.print(quote(prop.typeinfo().name));
			p.print(colon);
		}
		p.print(prop.getJsonValue());
		return;
	}

	// If required, print closing brace
	if(name) {
		if(pretty && object.streamPos) {
			newline();
			p.print(indent);
		}
		p.print(isObject ? '}' : ']');
	}

	--nesting;
}

void DataStream::fillStream(Print& p)
{
	if(done) {
		return;
	}
	auto newline = [&]() {
		if(pretty) {
			p.println();
		}
	};

	if(!store) {
		if(storeIndex == 0) {
			stream << '{';
		}
		store = db->getStore(storeIndex);
		objects[0] = Object(store->typeinfo(), *store);
		nesting = 0;
	}

	printObject(p);
	if(nesting != 255) {
		return;
	}
	store.reset();

	if(!db) {
		done = true;
		return;
	}

	++storeIndex;
	if(storeIndex < db->typeinfo.storeCount) {
		p.print(',');
		newline();
		return;
	}

	newline();
	p.print('}');
	newline();
	done = true;
}

uint16_t DataStream::readMemoryBlock(char* data, int bufSize)
{
	if(bufSize <= 0) {
		return 0;
	}

	if(stream.available() == 0) {
		fillStream(stream);
		maxUsedBuffer = std::max(maxUsedBuffer, size_t(stream.available()));
	}

	return stream.readMemoryBlock(data, bufSize);
}

bool DataStream::seek(int len)
{
	if(len <= 0) {
		return false;
	}

	if(!stream.seek(len)) {
		return false;
	}

	if(stream.available() == 0) {
		stream.clear();
		fillStream(stream);
		maxUsedBuffer = std::max(maxUsedBuffer, size_t(stream.available()));
	}

	return true;
}

} // namespace ConfigDB::Json
