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
	auto msg = JsonRPC::importMessage(db, jsonString);

	RpcData::Root root(db);

	Serial << "type " << root.getTagString() << ", id " << msg.id << endl << root << endl;

	switch(root.getTag()) {
	case RpcData::Root::Tag::Params: {
		auto params = root.asParams();

		// m_puts("params: ");
		// m_nputs(msg.params.start, msg.params.length);
		// m_puts("\n");

		break;
	}

	case RpcData::Root::Tag::Result:
		// m_puts("result: ");
		// m_nputs(msg.result.start, msg.result.length);
		// m_puts("\n");

		break;

	case RpcData::Root::Tag::Error:
		// m_puts("error: ");
		// m_nputs(msg.error.start, msg.error.length);
		// m_puts("\n");
		break;
	}

	return msg;
}

void rpcExport(RpcData& db, int id)
{
	Serial << "EXPORT:" << endl;
	JsonRPC::exportMessage(db, id, Serial);
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
	rpcExport(db, msg.id);

	Serial << endl << "IMPORT request2" << endl;
	msg = rpcImport(db, Message::request2);
	rpcExport(db, msg.id);

	Serial << endl << "IMPORT error" << endl;
	msg = rpcImport(db, Message::error);
	rpcExport(db, msg.id);

	Serial << endl << "IMPORT response" << endl;
	msg = rpcImport(db, Message::response);
	rpcExport(db, msg.id);

	Serial << endl << endl;
}
