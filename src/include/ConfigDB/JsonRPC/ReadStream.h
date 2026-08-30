/****
 * ConfigDB/JsonRPC/ReadStream.h
 *
 * Copyright 2026 mikee47 <mike@sillyhouse.net>
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

#include <ConfigDB/Json/ReadStream.h>
#include "Message.h"

namespace JsonRPC
{
using namespace ConfigDB;

class ReadStream : public IDataSourceStream
{
public:
	ReadStream(Database& db, const Message& msg, bool pretty = false) : store(db.openStore(0)), msg(msg), pretty(pretty)
	{
		if(msg.kind == Message::Kind::none || !store) {
			state = State::done;
		}
	}

	static size_t print(Database& db, const Message& msg, Print& p, bool pretty = false);

	size_t printHeader(Print& p);

	size_t printFooter(Print& p);

	bool isValid() const override
	{
		return true;
	}

	uint16_t readMemoryBlock(char* data, int bufSize) override;

	bool seek(int len) override
	{
		return stream ? stream->seek(len) : false;
	}

	bool isFinished() override
	{
		return state == State::done;
	}

	MimeType getMimeType() const override
	{
		return MimeType::JSON;
	}

protected:
	enum class State {
		header,
		body,
		footer,
		done,
	};
	StoreRef store;
	const Message msg;
	Object request;
	Object body;
	std::unique_ptr<IDataSourceStream> stream;
	State state{};
	const bool pretty;
};

} // namespace JsonRPC
