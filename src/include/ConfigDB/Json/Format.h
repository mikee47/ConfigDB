/**
 * ConfigDB/Format/Json.h
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

#pragma once

#include "../Format.h"
#include "ReadStream.h"
#include "WriteStream.h"

namespace ConfigDB::Json
{
class Format : public ConfigDB::Format
{
public:
	DEFINE_FSTR_LOCAL(fileExtension, ".json")

	std::unique_ptr<IDataSourceStream> createExportStream(Database& db) const
	{
		return std::make_unique<ReadStream>(db, pretty);
	}

	std::unique_ptr<IDataSourceStream> createExportStream(std::shared_ptr<Store> store) const
	{
		return std::make_unique<ReadStream>(store, pretty);
	}

	size_t exportToStream(const Object& object, Print& output) const
	{
		Printer printer(output, object, pretty, Printer::RootStyle::braces);
		size_t n{0};
		do {
			n += printer();
		} while(!printer.isDone());
		return n;
	}

	size_t exportToStream(Database& database, Print& output) const
	{
		return ReadStream::print(database, output, pretty);
	}

	std::unique_ptr<ReadWriteStream> createImportStream(Database& db) const
	{
		return std::make_unique<WriteStream>(db);
	}

	std::unique_ptr<ReadWriteStream> createImportStream(std::shared_ptr<Store> store) const
	{
		return std::make_unique<WriteStream>(store);
	}

	bool importFromStream(Store& store, Stream& source) const
	{
		store.clear();
		auto status = WriteStream::parse(store, source);

		if(status == JSON::Status::EndOfDocument) {
			return true;
		}

		debug_w("JSON load '%s': %s", store.getName().c_str(), toString(status).c_str());
		return false;
	}

	bool importFromStream(Database& database, Stream& source) const
	{
		auto status = WriteStream::parse(database, source);

		if(status == JSON::Status::EndOfDocument) {
			return true;
		}

		debug_w("JSON load '%s': %s", database.getName().c_str(), toString(status).c_str());
		return false;
	}

	String getFileExtension() const override
	{
		return fileExtension;
	}

	MimeType getMimeType() const override
	{
		return MimeType::JSON;
	}

	void setPretty(bool pretty)
	{
		this->pretty = pretty;
	}

private:
	bool pretty{false};
};

extern Format format;

} // namespace ConfigDB::Json
