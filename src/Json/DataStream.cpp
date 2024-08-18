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
void DataStream::printObject()
{
	if(nesting == 0) {
		state.objects[0] = Object(store->typeinfo(), *store);
		state.itemCounts[0] = 0;
		++nesting;
	}
	auto& object = state.objects[nesting - 1];
	auto& itemCount = state.itemCounts[nesting - 1];
	auto name = &object.typeinfo().name;
	if(nesting == 1 && storeIndex == 0) {
		name = nullptr;
	}

	bool isObject = (object.typeinfo().type <= ObjectType::Object);

	auto newline = [&]() {
		if(pretty) {
			stream.println();
		}
	};

	String indent;
	if(pretty) {
		indent.pad(nesting * 2);
	}
	const char* colon = pretty ? ": " : ":";

	if(itemCount == 0 && name) {
		if(name->length()) {
			if(pretty) {
				stream.print(indent);
			}
			stream.print(quote(*name));
			stream.print(colon);
		}
		stream.print(isObject ? '{' : '[');
	}

	auto objectCount = object.getObjectCount();
	unsigned index = itemCount;
	if(index < objectCount) {
		auto obj = const_cast<Object&>(object).getObject(index);
		if(itemCount++) {
			stream.print(',');
		}
		newline();
		if(pretty && object.typeinfo().type == ObjectType::ObjectArray) {
			stream.print(indent);
			stream.print("  ");
		}
		state.objects[nesting] = obj;
		state.itemCounts[nesting] = 0;
		++nesting;
		return;
	}

	index -= objectCount;

	auto propertyCount = object.getPropertyCount();
	if(index < propertyCount) {
		auto prop = const_cast<Object&>(object).getProperty(index);
		if(itemCount++) {
			stream.print(',');
		}
		newline();
		if(pretty) {
			stream.print(indent);
			stream.print("  ");
		}
		if(prop.typeinfo().name.length()) {
			stream.print(quote(prop.typeinfo().name));
			stream.print(colon);
		}
		stream.print(prop.getJsonValue());
		return;
	}

	if(name) {
		if(pretty && itemCount) {
			newline();
			stream.print(indent);
		}
		stream.print(isObject ? '}' : ']');
	}

	--nesting;
}

void DataStream::fillStream(Print& stream)
{
	if(done) {
		return;
	}
	auto newline = [&]() {
		if(pretty) {
			stream << endl;
		}
	};

	if(nesting > 0) {
		printObject();
		if(nesting > 0) {
			return;
		}
		++storeIndex;
	}

	if(storeIndex == 0) {
		stream << '{';
	}
	store.reset();
	store = db->getStore(storeIndex);
	if(store) {
		if(storeIndex > 0) {
			stream << ',';
			newline();
		}
		printObject();
		return;
	}
	newline();
	stream << '}';
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
