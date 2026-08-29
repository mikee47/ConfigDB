/****
 * ConfigDB/JsonRPC/WriteStream.h
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

#include <ConfigDB/Json/WriteStream.h>
#include "Message.h"

namespace JsonRPC
{
class WriteStream : public ConfigDB::Json::WriteStream
{
public:
	/**
	 * @brief Callback interface provided by Application
	 */
	struct Callback {
		/**
		 * @brief Get a writeable object for the data with a given request ID
		 * @param requestId ID specified in received response or error message
		 * @param isError true if an error was received, false for a regular response
		 * @retval ConfigDB::Object Object instance to write message content
		 */
		virtual ConfigDB::Object getObject(int requestId, bool isError) = 0;

		virtual ConfigDB::Object getObject(const String& method) = 0;
	};

	using Element = JSON::Element;

	WriteStream(Message& msg, Callback& callback) : ConfigDB::Json::WriteStream(), msg(msg), callback(callback)
	{
	}

	bool isReparseRequired() const
	{
		return repeatParse;
	}

	void reset()
	{
		parser.reset();
		jsonStatus = JSON::Status::Ok;
		repeatParse = false;
	}

protected:
	bool startElement(const Element& element) override;

	Message& msg;
	Callback& callback;
	bool haveMethod{false};
	bool haveId{false};
	bool repeatParse{false};
};

} // namespace JsonRPC
