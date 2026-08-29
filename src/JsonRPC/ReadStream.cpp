#include <ConfigDB/JsonRPC/ReadStream.h>

namespace JsonRPC
{
size_t ReadStream::print(Database& db, const Message& msg, Print& p, bool pretty)
{
	size_t n{0};

	ReadStream stream(db, msg, pretty);
	n += stream.printHeader(p);

	if(stream.body) {
		Json::Printer printer(p, stream.body, pretty, Json::Printer::RootStyle::braces);
		do {
			n += printer();
		} while(!printer.isDone());
	}

	n += stream.printFooter(p);

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

	auto setBody = [&](const String& ident) {
		body = request.findObject(ident.c_str(), ident.length());
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

	auto& root = reinterpret_cast<Union&>(*store);
	request = root.getObject(0);
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
		n += p.print(root.getTagString());
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
				stream = std::make_unique<Json::ReadStream>(store, body, ExportOptions{.pretty = pretty});
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
