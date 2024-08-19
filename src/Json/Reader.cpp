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
#include <ConfigDB/Json/Printer.h>

namespace ConfigDB::Json
{
Reader reader;

size_t Reader::printObjectTo(const Object& object, const FlashString* name, unsigned nesting, Print& p) const
{
	Printer printer(p, object, format, Printer::RootStyle::braces);
	size_t n{0};
	do {
		n += printer();
	} while(!printer.isDone());
	return n;
}

std::unique_ptr<IDataSourceStream> Reader::createStream(Database& db) const
{
	return std::make_unique<DataStream>(db, format);
}

std::unique_ptr<IDataSourceStream> Reader::createStream(std::shared_ptr<Store> store) const
{
	return std::make_unique<DataStream>(store, format);
}

} // namespace ConfigDB::Json
