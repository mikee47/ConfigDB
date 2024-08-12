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

#include <ConfigDB/Json/DataStream.h>
#include <ConfigDB/Json/Store.h>

namespace ConfigDB
{
void DataStream::fillStream()
{
	stream.clear();
	if(done) {
		return;
	}
	auto format = db.getFormat();
	auto newline = [&]() {
		if(format == Format::Pretty) {
			stream << endl;
		}
	};
	if(storeIndex == 0) {
		stream << '{';
		newline();
	}
	auto store = db.getStore(storeIndex);
	if(store) {
		if(storeIndex > 0) {
			stream << ',';
			newline();
		}
		store->printTo(stream, 1);
		++storeIndex;
		return;
	}
	newline();
	stream << '}' << endl;
	done = true;
}

uint16_t DataStream::readMemoryBlock(char* data, int bufSize)
{
	if(bufSize <= 0) {
		return 0;
	}

	if(storeIndex == 0) {
		fillStream();
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
		fillStream();
	}

	return true;
}

} // namespace ConfigDB
