#include <SmingCore.h>
#include <RpcData.h>
#include "JsonRPC.h"
#include <Data/Stream/MemoryDataStream.h>

#ifdef ENABLE_MALLOC_COUNT
#include <malloc_count.h>
#endif

namespace
{
namespace Message
{
IMPORT_FSTR(request, PROJECT_DIR "/json/request.json")
IMPORT_FSTR(request2, PROJECT_DIR "/json/request2.json")
IMPORT_FSTR(response, PROJECT_DIR "/json/response.json")
IMPORT_FSTR(error, PROJECT_DIR "/json/error.json")
} // namespace Message

JsonRPC::Message rpcImport(RpcData& db, const String& jsonString)
{
	auto getRequestTag = [](int id) -> int { return 1; };

	auto msg = JsonRPC::importMessage(db, jsonString, getRequestTag);

	RpcData::Root root(db);

	Serial << "RPC " << toString(msg.kind) << ": method " << root.getTagString() << ", id " << msg.id << endl;

	using Tag = RpcData::Root::Tag;
	switch(root.getTag()) {
	case Tag::ColorEvent: {
		auto obj = root.asColorEvent();
		using Kind = JsonRPC::Message::Kind;
		switch(msg.kind) {
		case Kind::none:
			break;
		case Kind::params:
			Serial << obj.params;
			break;
		case Kind::result:
			Serial << obj.result;
			break;
		case Kind::error:
			Serial << obj.error;
			break;
		}

		break;
	}

	case RpcData::Root::Tag::None:

		break;
	}

	Serial << endl;

	return msg;
}

void rpcExport(RpcData& db, const JsonRPC::Message& msg)
{
	Serial << "EXPORT:" << endl;
	JsonRPC::exportMessage(db, msg, Serial);
}

} // namespace

void init()
{
	Serial.begin(COM_SPEED_SERIAL);
	Serial.systemDebugOutput(true);

#ifdef ARCH_HOST
	fileSetFileSystem(&IFS::Host::getFileSystem());
#else
	spiffs_mount();
#endif

	RpcData db("RPC");
	// Don't commit parsed data to filesystem
	RpcData::Root::onCommit(db, [](auto params) { params.clearDirty(); });

	Serial << endl << "IMPORT request" << endl;
	auto msg = rpcImport(db, Message::request);
	rpcExport(db, msg);

	Serial << endl << "IMPORT request2" << endl;
	msg = rpcImport(db, Message::request2);
	rpcExport(db, msg);

	Serial << endl << "IMPORT response" << endl;
	msg = rpcImport(db, Message::response);
	rpcExport(db, msg);

	Serial << endl << "IMPORT error" << endl;
	msg = rpcImport(db, Message::error);
	rpcExport(db, msg);

	Serial << endl << endl;
}
