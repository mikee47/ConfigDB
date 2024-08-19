/**
 * ConfigDB/DataStream.h
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

#include "../Database.h"
#include <Data/WebConstants.h>
#include <Data/Stream/MemoryDataStream.h>
#include "Printer.h"

namespace ConfigDB::Json
{
/**
 * @brief Forward-reading stream for serializing entire database contents
 */
class DataStream : public IDataSourceStream
{
public:
	DataStream(Database& db, Format format) : db(&db), format(format)
	{
	}

	DataStream(std::shared_ptr<Store> store, Format format) : store(store), format(format)
	{
	}

	bool isValid() const override
	{
		return true;
	}

	uint16_t readMemoryBlock(char* data, int bufSize) override;

	bool seek(int len) override;

	bool isFinished() override
	{
		return done && stream.isFinished();
	}

	String getName() const override
	{
		return db ? db->getName() : nullptr;
	}

	MimeType getMimeType() const override
	{
		return MimeType::JSON;
	}

private:
	void fillStream();

	Database* db{};
	std::shared_ptr<Store> store;
	Printer printer;
	MemoryDataStream stream;
	Format format;
	uint8_t storeIndex{0};
	bool done{false};
};

} // namespace ConfigDB::Json
