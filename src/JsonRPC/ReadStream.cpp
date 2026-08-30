/****
 * ConfigDB/JsonRPC/ReadStream.cpp
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

#include <ConfigDB/JsonRPC/ReadStream.h>
#include <ConfigDB/Json/Printer.h>
#include <Data/Stream/MemoryDataStream.h>

namespace JsonRPC
{
size_t ReadStream::print(const Message& msg, const ConfigDB::Object& body, Print& p, bool pretty)
{
	size_t n{0};

	ReadStream rs(msg, pretty);
	n += rs.printHeader(p);

	if(body) {
		Json::Printer printer(p, body, pretty, Json::Printer::RootStyle::braces);
		do {
			n += printer();
		} while(!printer.isDone());
	}

	n += rs.printFooter(p);

	return n;
}

size_t ReadStream::printHeader(Print& p)
{
	size_t n{0};

	const char* colon = pretty ? ": " : ":";

	n += p.print('{');
	if(pretty) {
		n += p.println();
	}
	n += p.print(_F("\"jsonrpc\""));
	n += p.print(colon);
	n += p.print(_F("\"2.0\""));

	if(msg.kind != Message::Kind::notification) {
		n += p.print(',');
		if(pretty) {
			n += p.println();
		}
		n += p.print(_F("\"id\""));
		n += p.print(colon);
		n += p.print(msg.id);
	}

	auto setBody = [&](const FSTR::String& ident) {
		if(!body) {
			return;
		}
		n += p.print(',');
		if(pretty) {
			n += p.println();
		}
		n += p.print('"');
		n += p.print(ident);
		n += p.print('"');
		n += p.print(colon);
	};

	switch(msg.kind) {
	case Message::Kind::request:
	case Message::Kind::notification: {
		n += p.print(',');
		if(pretty) {
			n += p.println();
		}
		n += p.print(_F("\"method\""));
		n += p.print(colon);
		n += p.print('"');
		n += p.print(msg.method);
		n += p.print('"');
		setBody(FS_params);
		break;
	}
	case Message::Kind::result: {
		setBody(FS_result);
		break;
	}
	case Message::Kind::error: {
		setBody(FS_error);
		break;
	}
	case Message::Kind::none:
		break;
	}

	return n;
}

size_t ReadStream::printFooter(Print& p)
{
	if(pretty) {
		return p.print(_F("\r\n}\r\n"));
	}
	return p.print('}');
}

uint16_t ReadStream::readMemoryBlock(char* data, int bufSize)
{
	if(state == State::done || bufSize <= 0) {
		return 0;
	}

	if(!stream || stream->isFinished()) {
		stream.reset();

		switch(state) {
		case State::header: {
			auto mem = std::make_unique<MemoryDataStream>();
			printHeader(*mem);
			stream.reset(mem.release());
			state = State::body;
			break;
		}

		case State::body:
			state = State::footer;
			if(body) {
				stream = std::move(body);
				break;
			}
			[[fallthrough]];

		case State::footer: {
			auto mem = std::make_unique<MemoryDataStream>();
			printFooter(*mem);

			stream.reset(mem.release());
			state = State::done;
			break;
		}

		case State::done:
			return 0;
		}
	}

	return stream->readMemoryBlock(data, bufSize);
}

} // namespace JsonRPC
