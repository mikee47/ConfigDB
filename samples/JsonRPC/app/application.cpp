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
			RpcData::Root::OuterUpdater root(database);
			auto event = root.toColorEvent();
			return {root.store, event.params};
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

			RpcData::Root::OuterUpdater upd(database);
			auto event = upd.toColorEvent();
			return {upd.store, event.result};
		}

		ConfigDB::ObjectUpdateRef getErrorObject(int requestId) override
		{
			RpcData::Root::OuterUpdater upd(database);
			auto event = upd.toColorEvent();
			return {upd.store, event.error};
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
			Serial << RpcData::Root(database).asColorEvent().params;
		}
		break;
	case Kind::result:
	case Kind::notification:
		Serial << RpcData::Root(database).asColorEvent().result;
		break;
	case Kind::error:
		Serial << RpcData::Root(database).asColorEvent().error;
		break;
	}

	Serial << endl;

	return msg;
}

#if 1

void rpcExport(const JsonRPC::Message& msg, const ConfigDB::ObjectRef& body)
{
	Serial << "EXPORT:" << endl;
	JsonRPC::exportMessage(msg, body.object, Serial);
	Serial << endl << endl;
}

#else

void rpcExport(const JsonRPC::Message& msg, const ConfigDB::ObjectRef& body)
{
	Serial << "EXPORT:" << endl;

	auto stream = new JsonRPC::ReadStream(msg, body);
	System.queueCallback(
		[](void* param) {
			auto stream = static_cast<IDataSourceStream*>(param);
			Serial.copyFrom(stream);
			Serial << endl << endl;
			delete stream;
		},
		stream);
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
	// RpcData::ColorEvent::onCommit(database, [](auto params) {
	// 	params.clearDirty();
	// 	Serial << "** COLOR EVENT ** " << endl << params << endl;
	// });

	{
		Serial << endl << "IMPORT request" << endl;
		auto msg = rpcImport(json::request);
		RpcData::Root root(database);
		rpcExport(msg, {root.store, root.asColorEvent().params});
	}

	{
		Serial << endl << "IMPORT request2" << endl;
		auto msg = rpcImport(json::request2);
		RpcData::Root root(database);
		rpcExport(msg, {root.store, root.asColorEvent().params});
	}

	{
		Serial << endl << "IMPORT response" << endl;
		auto msg = rpcImport(json::response);
		RpcData::Root root(database);
		rpcExport(msg, {root.store, root.asColorEvent().result});
	}

	{
		Serial << endl << "IMPORT error" << endl;
		auto msg = rpcImport(json::error);
		RpcData::Root root(database);
		rpcExport(msg, {root.store, root.asColorEvent().error});
	}

	Serial << endl << endl;
}
