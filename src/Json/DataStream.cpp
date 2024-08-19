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

namespace ConfigDB::Json
{
void DataStream::fillStream()
{
	if(done) {
		return;
	}

	if(db) {
		if(!store) {
			if(storeIndex == 0) {
				stream << '{';
			}
			store = db->getStore(storeIndex);
			auto style = storeIndex == 0 ? Printer::RootStyle::hidden : Printer::RootStyle::normal;
			printer = Printer(stream, *store, format, style);
		}
	} else if(!printer) {
		printer = Printer(stream, *store, format, Printer::RootStyle::normal);
	}

	printer();
	if(!printer.isDone()) {
		return;
	}
	store.reset();

	if(!db) {
		done = true;
		return;
	}

	++storeIndex;
	if(storeIndex < db->typeinfo.storeCount) {
		stream.print(',');
		printer.newline();
		return;
	}

	printer.newline();
	stream.print('}');
	printer.newline();
	done = true;
}

uint16_t DataStream::readMemoryBlock(char* data, int bufSize)
{
	if(bufSize <= 0) {
		return 0;
	}

	if(stream.available() == 0) {
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
		stream.clear();
		fillStream();
	}

	return true;
}

} // namespace ConfigDB::Json
