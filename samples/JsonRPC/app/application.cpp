#include <SmingCore.h>
#include <RpcData.h>
#include <ConfigDB/JsonRPC/JsonRPC.h>
#include <WVector.h>

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

/**
 * Application should keep note of outstanding requests in some way
 * so incoming responses can be tied to the originating request.
 */
struct RequestInfo {
	/**
	 * @brief Method identifier, numeric is more efficient than string
	 */
	unsigned method;
};

HashMap<int /* Request ID */, RequestInfo> requestMap;

JsonRPC::Message rpcImport(const String& jsonString)
{
	class CallbackImpl : public JsonRPC::WriteStream::Callback
	{
	public:
		ConfigDB::ObjectUpdateRef getParamsObject(const String& method) override
		{
			return database.getObjectForUpdate(method);
		}

		ConfigDB::ObjectUpdateRef getResultObject(int requestId) override
		{
			// Result content generally depends on the request
			int i = requestMap.indexOf(requestId);
			if(i < 0) {
				// Request not found
			} else {
				auto requestIndex = requestMap.valueAt(i);
				// ...
			}

			return RpcData::Root::Result::OuterUpdater(database);
		}

		ConfigDB::ObjectUpdateRef getErrorObject(int requestId) override
		{
			return RpcData::Root::Error::OuterUpdater(database);
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
			Serial << RpcData::ColorEvent(database);
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

	/*
	In this example each event has its own store so this commit callback is invoked
	only when the event data has been updated.
	What's missing here is the request ID.
	*/
	RpcData::ColorEvent::onCommit(database, [](auto params) {
		params.clearDirty();
		Serial << "** COLOR EVENT ** " << endl << params << endl;
	});

	{
		Serial << endl << "IMPORT request" << endl;
		auto msg = rpcImport(json::request);
		rpcExport(msg, RpcData::ColorEvent(database));
	}

	{
		Serial << endl << "IMPORT request2" << endl;
		auto msg = rpcImport(json::request2);
		rpcExport(msg, RpcData::ColorEvent(database));
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
