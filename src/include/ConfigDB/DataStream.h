#pragma once

#include "ConfigDB.h"
#include <Data/WebConstants.h>
#include <Data/Stream/MemoryDataStream.h>

namespace ConfigDB
{
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
	void reset();

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
	uint8_t storeIndex{0};
	uint8_t objectIndex{0};
	enum class State {
		header,
		object,
		done,
	} state = State::header;
	uint8_t segIndex{0}; // nesting level
};

} // namespace ConfigDB