/**
 * ConfigDB/Json/ReadStream.cpp
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

#include "ReadStream.h"

namespace ConfigDB::Json
{
size_t ReadStream::print(Database& db, Print& p, bool pretty)
{
	ReadStream rs(db, pretty);
	size_t n{0};
	while(!rs.done) {
		n += rs.fillStream(p);
	}
	return n;
}

size_t ReadStream::fillStream(Print& p)
{
	if(done) {
		return 0;
	}

	size_t n{0};

	if(db) {
		if(!store) {
			if(storeIndex == 0) {
				n += p.print('{');
			}
			store = db->openStore(storeIndex);
			auto style = storeIndex == 0 ? Printer::RootStyle::hidden : Printer::RootStyle::normal;
			printer = Printer(p, *store, pretty, style);
		}
	} else if(!printer) {
		printer = Printer(p, *store, pretty, Printer::RootStyle::normal);
	}

	n += printer();
	if(!printer.isDone()) {
		return n;
	}
	store = {};

	if(!db) {
		done = true;
		return n;
	}

	++storeIndex;
	if(storeIndex < db->typeinfo.storeCount) {
		n += p.print(',');
		n += printer.newline();
		return n;
	}

	n += printer.newline();
	n += p.print('}');
	n += printer.newline();
	done = true;
	return n;
}

uint16_t ReadStream::readMemoryBlock(char* data, int bufSize)
{
	if(bufSize <= 0) {
		return 0;
	}

	if(stream.available() == 0) {
		fillStream(stream);
	}

	return stream.readMemoryBlock(data, bufSize);
}

bool ReadStream::seek(int len)
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
	}

	return true;
}

} // namespace ConfigDB::Json
