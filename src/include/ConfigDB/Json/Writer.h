/**
 * ConfigDB/Json/Writer.h
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

#include "../Writer.h"

namespace ConfigDB::Json
{
DECLARE_FSTR(fileExtension)

class Writer : public ConfigDB::Writer
{
public:
	using ConfigDB::Writer::Writer;

	std::unique_ptr<ReadWriteStream> createStream(Database& db) const override;
	std::unique_ptr<ReadWriteStream> createStream(std::shared_ptr<Store> store) const override;

	bool loadFromStream(Store& store, Stream& source) override;
	bool loadFromStream(Database& database, Stream& source) override;

	String getFileExtension() const override
	{
		return fileExtension;
	}
};

extern Writer writer;

} // namespace ConfigDB::Json
