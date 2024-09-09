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

	WriteStream(Database& database) : database(&database), parser(this)
	{
	}

	WriteStream(StoreUpdateRef& store, Object& object) : store(store), info{object}, parser(this)
	{
	}

	WriteStream(Object& object) : info{object}, parser(this)
	{
	}

	~WriteStream();

	static Status parse(Database& database, Stream& source);

	static Status parse(Object& object, Stream& source);

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
		return database ? database->getName() : info[0].getName();
	}

	MimeType getMimeType() const override
	{
		return MimeType::JSON;
	}

	Status getStatus() const override;

private:
	bool startElement(const JSON::Element& element) override;

	bool endElement(const JSON::Element&) override
	{
		return true;
	}

	bool locateStoreOrRoot(const JSON::Element& element);
	bool handleSelector(const JSON::Element& element, const char* sel);
	bool setProperty(const JSON::Element& element, Object& object, Property prop);
	bool handleError(FormatError err, Object& object, const String& arg);
	bool handleError(FormatError err, const String& arg);

private:
	Database* database{};
	StoreUpdateRef store;
	Object info[JSON::StreamingParser::maxNesting]{};
	ObjectArray arrayParent; ///< Temporary required when using selectors
	JSON::StaticStreamingParser<1024> parser;
	JSON::Status jsonStatus{};
	Status status;
};

} // namespace ConfigDB::Json
