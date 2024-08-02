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

#include "Database.h"
#include <Data/WebConstants.h>
#include <Data/Stream/MemoryDataStream.h>

namespace ConfigDB
{
/**
 * @brief Forward-reading stream for serializing entire database contents
 */
class DataStream : public IDataSourceStream
{
public:
	DataStream(Database& db) : db(db)
	{
		reset();
	}

	/**
	 * @brief Reset back to start
	 * @note Handy if you want to re-use this stream to send it somewhere else
	 */
	void reset()
	{
		state = State::header;
	}

	bool isValid() const override
	{
		return true;
	}

	uint16_t readMemoryBlock(char* data, int bufSize) override;

	bool seek(int len) override;

	bool isFinished() override
	{
		return state == State::done && stream.isFinished();
	}

	String getName() const override
	{
		return db.getName();
	}

	MimeType getMimeType() const override
	{
		return MimeType::JSON;
	}

private:
	void fillStream();

	Database& db;
	std::shared_ptr<Store> store;
	MemoryDataStream stream;
	uint8_t storeIndex;
	uint8_t propertyIndex;
	enum class State {
		header,
		object,
		done,
	} state;
};

} // namespace ConfigDB
