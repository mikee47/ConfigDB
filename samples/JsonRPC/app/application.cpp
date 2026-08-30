#include <SmingCore.h>
#include <RpcData.h>
#include <ConfigDB/JsonRPC/JsonRPC.h>
#include <WHashMap.h>

#ifdef ENABLE_MALLOC_COUNT
#include <malloc_count.h>
#endif

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
	using Tag = RpcData::Root::Tag;

	class CallbackImpl : public JsonRPC::WriteStream::Callback
	{
	public:
		CallbackImpl(RpcData::Root& root) : root(root.update())
		{
			root.resetToDefaults();
			map[1] = Tag::ColorEvent;
		}

		ConfigDB::Object getObject(int requestId, bool isError) override
		{
			int i = map.indexOf(requestId);
			if(i < 0) {
				return {};
			}
			auto tag = map.valueAt(i);
			root.setTag(tag);
			request = root.getObject(0);
			return request.findObject(isError ? "error" : "result");
		}

		ConfigDB::Object getObject(const String& method) override
		{
			request = root.findObject(method);
			return request.findObject("params");
		}

	private:
		RpcData::Root::OuterUpdater root;
		ConfigDB::Object request;
		HashMap<int, Tag> map;
	};

	RpcData::Root root(database);
	CallbackImpl callbacks(root);
	auto msg = JsonRPC::importMessage(jsonString, callbacks);

	Serial << "RPC " << toString(msg.kind) << ": method " << root.getTagString() << ", id " << msg.id << endl;

	switch(root.getTag()) {
	case Tag::ColorEvent: {
		auto obj = root.asColorEvent();
		using Kind = JsonRPC::Message::Kind;
		switch(msg.kind) {
		case Kind::none:
			break;
		case Kind::request:
			Serial << obj.params;
			break;
		case Kind::result:
		case Kind::notification:
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

void rpcExport(const JsonRPC::Message& msg, const ConfigDB::Object& body)
{
	Serial << "EXPORT:" << endl;
	JsonRPC::exportMessage(msg, body, Serial);
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

	// Don't commit parsed data to filesystem
	RpcData::Root::onCommit(database, [](auto params) { params.clearDirty(); });

	{
		Serial << endl << "IMPORT request" << endl;
		auto msg = rpcImport(json::request);
		auto body = RpcData::Root(database).asColorEvent().params;
		rpcExport(msg, body);
	}

	{
		Serial << endl << "IMPORT request2" << endl;
		auto msg = rpcImport(json::request2);
		auto body = RpcData::Root(database).asColorEvent().params;
		rpcExport(msg, body);
	}

	{
		Serial << endl << "IMPORT response" << endl;
		auto msg = rpcImport(json::response);
		auto body = RpcData::Root(database).asColorEvent().result;
		rpcExport(msg, body);
	}

	{
		Serial << endl << "IMPORT error" << endl;
		auto msg = rpcImport(json::error);
		auto body = RpcData::Root(database).asColorEvent().error;
		rpcExport(msg, body);
	}

	Serial << endl << endl;
}
