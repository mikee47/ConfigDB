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
class ReadStream : public ExportStream
{
public:
	ReadStream(Database& db, const ExportOptions& options = {}) : db(&db), options(options)
	{
	}

	ReadStream(StoreRef& store, const Object& object, const ExportOptions& options = {})
		: store(store), printer(stream, object, options.pretty, std::max(RootStyle::braces, options.rootStyle)),
		  options(options)
	{
	}

	static size_t print(Database& db, Print& p, const ExportOptions& options);

	bool isValid() const override
	{
		return true;
	}

	uint16_t readMemoryBlock(char* data, int bufSize) override;

	int seekFrom(int offset, SeekOrigin origin) override;

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

	Status getStatus() const override
	{
		return Status{};
	}

	virtual Options getOptions() const override
	{
		return Options{};
	}

	virtual void setOptions(const Options& options) override
	{
		printer.setRootStyle(options.rootStyle, options.rootName);
	}

private:
	size_t fillStream(Print& p);

	Database* db{};
	StoreRef store;
	Printer printer;
	MemoryDataStream stream;
	const ExportOptions options;
	unsigned streamPos{0};
	uint8_t storeIndex{0};
	bool done{false};
};

} // namespace ConfigDB::Json
