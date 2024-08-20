/**
 * ConfigDB/Json/Reader.h
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

#include "../Reader.h"
#include "Printer.h"

namespace ConfigDB::Json
{
DECLARE_FSTR(fileExtension)

class Reader : public ConfigDB::Reader
{
public:
	using ConfigDB::Reader::Reader;

	std::unique_ptr<IDataSourceStream> createStream(Database& db) const override;
	std::unique_ptr<IDataSourceStream> createStream(std::shared_ptr<Store> store) const override;

	size_t saveToStream(const Object& object, Print& stream) const override;
	size_t saveToStream(Database& database, Print& stream) override;

	String getFileExtension() const override
	{
		return fileExtension;
	}

	void setFormat(Format format)
	{
		this->format = format;
	}

private:
	Format format{};
};

extern Reader reader;

} // namespace ConfigDB::Json
