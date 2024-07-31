/**
 * ConfigDB/DataStream.cpp
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

#include "include/ConfigDB/DataStream.h"
#include "include/ConfigDB/Store.h"
#include "include/ConfigDB/Object.h"

namespace ConfigDB
{
void DataStream::fillStream()
{
	stream.clear();
	if(state == State::done) {
		return;
	}
	auto format = db.getFormat();
	auto newline = [&]() {
		if(format == Format::Pretty) {
			stream << endl;
		}
	};
	if(state == State::header) {
		stream << '{';
		newline();
		state = State::object;
		storeIndex = 0;
		objectIndex = 0;
		store.reset();
		store = db.getStore(0);
	}
	while(store) {
		if(objectIndex == 0) {
			if(storeIndex > 0) {
				stream << ',';
				newline();
			}
			if(auto& name = store->getName()) {
				stream << '"' << name << "\":";
			}
		}
		auto object = store->getObject(objectIndex);
		if(!object) {
			++storeIndex;
			store.reset();
			store = db.getStore(storeIndex);
			objectIndex = 0;
			continue;
		}
		if(objectIndex > 0) {
			stream << ',';
			newline();
		}
		if(auto& name = object->getName()) {
			stream << '"' << name << "\":";
		}
		stream << *object;
		++objectIndex;
		return;
	}
	newline();
	stream << '}' << endl;
	state = State::done;
}

uint16_t DataStream::readMemoryBlock(char* data, int bufSize)
{
	if(bufSize <= 0) {
		return 0;
	}

	if(state == State::header) {
		fillStream();
	}

	return stream.readMemoryBlock(data, bufSize);
}

bool DataStream::seek(int len)
{
	if(len <= 0) {
		return false;
	}

	stream.seek(len);
	if(stream.available() == 0) {
		fillStream();
	}
	return true;
}

} // namespace ConfigDB
