#pragma once

#include <ConfigDB/Json/WriteStream.h>
#include "Message.h"

namespace JsonRPC
{
/**
 * @brief Applications should implement this method
 * @param id Received message RPC ID
 * @retval int Root tag index for corresponding request, -1 if unknown
 */
using GetRequestTag = Delegate<int(int id)>;

class WriteStream : public ConfigDB::Json::WriteStream
{
public:
	using Element = JSON::Element;

	WriteStream(ConfigDB::Object& obj, Message& msg, GetRequestTag& getRequestTag)
		: ConfigDB::Json::WriteStream(obj), root(static_cast<ConfigDB::Union&>(obj)), msg(msg),
		  getRequestTag(getRequestTag)
	{
		assert(root.typeIs(ConfigDB::ObjectType::Union));
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

	ConfigDB::Union& root;
	Message& msg;
	GetRequestTag getRequestTag;
	bool haveMethod{false};
	bool haveId{false};
	bool repeatParse{false};
};

} // namespace JsonRPC
