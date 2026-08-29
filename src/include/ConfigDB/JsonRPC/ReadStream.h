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
