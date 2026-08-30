#include <SmingCore.h>
#include <RpcData.h>
#include <ConfigDB/JsonRPC/JsonRPC.h>

namespace
{
namespace json
{
IMPORT_FSTR(request, PROJECT_DIR "/json/request.json")
IMPORT_FSTR(request2, PROJECT_DIR "/json/request2.json")
IMPORT_FSTR(response, PROJECT_DIR "/json/response.json")
IMPORT_FSTR(error, PROJECT_DIR "/json/error.json")
} // namespace json

RpcData database("RPC");

JsonRPC::Message rpcImport(const String& jsonString)
{
	class CallbackImpl : public JsonRPC::WriteStream::Callback
	{
	public:
		ConfigDB::ObjectUpdateRef getObject(int requestId, bool isError) override
		{
			if(isError) {
				return RpcData::Root::Error::OuterUpdater(database);
			}
			return RpcData::Root::Result::OuterUpdater(database);
		}

		ConfigDB::ObjectUpdateRef getObject(const String& method) override
		{
			if(method == "color_event") {
				return RpcData::Root::ColorEvent::OuterUpdater(database);
			}
			return {};
		}
	};

	CallbackImpl callbacks;
	auto msg = JsonRPC::importMessage(jsonString, callbacks);

	Serial << "RPC " << toString(msg.kind) << ": method " << msg.method << ", id " << msg.id << endl;

	using Kind = JsonRPC::Message::Kind;
	switch(msg.kind) {
	case Kind::none:
		break;
	case Kind::request:
		if(msg.method == "color_event") {
			Serial << RpcData::Root::ColorEvent(database);
		}
		break;
	case Kind::result:
	case Kind::notification:
		Serial << RpcData::Root::Result(database);
		break;
	case Kind::error:
		Serial << RpcData::Root::Error(database);
		break;
	}

	Serial << endl;

	return msg;
}

#if 1

void rpcExport(const JsonRPC::Message& msg, const ConfigDB::Object& body)
{
	Serial << "EXPORT:" << endl;
	JsonRPC::exportMessage(msg, body, Serial);
	Serial << endl << endl;
}

#else

template <typename T> void rpcExport(const JsonRPC::Message& msg, const T& body)
{
	Serial << "EXPORT:" << endl;

	auto stream = std::make_unique<JsonRPC::ReadStream>(msg, body);

	Serial.copyFrom(stream.get());
	Serial << endl << endl;
}

#endif

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

	// Don't commit parsed data to filesystem
	RpcData::Root::onCommit(database, [](auto params) { params.clearDirty(); });

	{
		Serial << endl << "IMPORT request" << endl;
		auto msg = rpcImport(json::request);
		rpcExport(msg, RpcData::Root::ColorEvent(database));
	}

	{
		Serial << endl << "IMPORT request2" << endl;
		auto msg = rpcImport(json::request2);
		rpcExport(msg, RpcData::Root::ColorEvent(database));
	}

	{
		Serial << endl << "IMPORT response" << endl;
		auto msg = rpcImport(json::response);
		rpcExport(msg, RpcData::Root::Result(database));
	}

	{
		Serial << endl << "IMPORT error" << endl;
		auto msg = rpcImport(json::error);
		rpcExport(msg, RpcData::Root::Error(database));
	}

	Serial << endl << endl;
}
