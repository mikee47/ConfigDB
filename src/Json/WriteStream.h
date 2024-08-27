/**
 * ConfigDB/Json/WriteStream.h
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
#include <JSON/StreamingParser.h>

namespace ConfigDB::Json
{
/**
 * @brief Stream for deserialising JSON into database
 */
class WriteStream : public ImportStream, private JSON::Listener
{
public:
	WriteStream() : parser(this)
	{
	}

	WriteStream(Database& db) : db(&db), parser(this)
	{
	}

	WriteStream(std::shared_ptr<Store> store, Object& object) : store(store), info{object}, parser(this)
	{
	}

	WriteStream(Object& object) : info{object}, parser(this)
	{
	}

	~WriteStream();

	static JSON::Status parse(Database& db, Stream& source);

	static JSON::Status parse(Object& object, Stream& source);

	bool isValid() const override
	{
		return true;
	}

	using ReadWriteStream::write;

	size_t write(const uint8_t* data, size_t size) override;

	uint16_t readMemoryBlock(char*, int) override
	{
		return 0;
	}

	int available() override
	{
		return 0;
	}

	bool seek(int) override
	{
		return false;
	}

	bool isFinished() override
	{
		return true;
	}

	String getName() const override
	{
		return db ? db->getName() : info[0].getName();
	}

	MimeType getMimeType() const override
	{
		return MimeType::JSON;
	}

private:
	bool startElement(const JSON::Element& element) override;

	bool endElement(const JSON::Element&) override
	{
		return true;
	}

private:
	Database* db{};
	std::shared_ptr<Store> store;
	Object info[JSON::StreamingParser::maxNesting]{};
	ObjectArray arrayParent; ///< Temporary required when using selectors
	JSON::StaticStreamingParser<1024> parser;
};

} // namespace ConfigDB::Json
