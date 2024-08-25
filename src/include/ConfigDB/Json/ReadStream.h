/**
 * ConfigDB/Json/ReadStream.h
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

#include <ConfigDB/Database.h>
#include <Data/Stream/MemoryDataStream.h>
#include "Printer.h"

namespace ConfigDB::Json
{
/**
 * @brief Forward-reading stream for serializing entire database contents
 */
class ReadStream : public IDataSourceStream
{
public:
	ReadStream(Database& db, bool pretty) : db(&db), pretty(pretty)
	{
	}

	ReadStream(std::shared_ptr<Store> store, bool pretty) : store(store), pretty(pretty)
	{
	}

	static size_t print(Database& db, Print& p, bool pretty);

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
		return db ? db->getName() : store ? store->getName() : nullptr;
	}

	MimeType getMimeType() const override
	{
		return MimeType::JSON;
	}

private:
	size_t fillStream(Print& p);

	Database* db{};
	std::shared_ptr<Store> store;
	Printer printer;
	MemoryDataStream stream;
	bool pretty;
	uint8_t storeIndex{0};
	bool done{false};
};

} // namespace ConfigDB::Json
