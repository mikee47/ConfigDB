#include "JsonRPC.h"
#include <ConfigDB/Database.h>
#include <ConfigDB/Json/WriteStream.h>
#include <JSON/StreamingParser.h>
#include <Data/Stream/LimitedMemoryStream.h>

namespace JsonRPC
{
class RpcStream : public ConfigDB::Json::WriteStream
{
public:
	using Element = JSON::Element;

	RpcStream(ConfigDB::Object& obj, Message& msg) : WriteStream(obj), root(obj), msg(msg)
	{
	}

	void reset()
	{
		parser.reset();
		jsonStatus = JSON::Status::Ok;
	}

protected:
	bool startElement(const Element& element) override
	{
		if(element.level < 1) {
			return true;
		}

		if(element.level > 1) {
			// We can only fully parse the message once the kind has been established
			if(msg.kind != Message::Kind::none) {
				return WriteStream::startElement(element);
			}
			return true;
		}

		if(element.keyIs("method")) {
			msg.method = element.as<String>();
			return true;
		}

		if(element.keyIs("id")) {
			msg.id = element.as<int>();
			return true;
		}

		if(element.keyIs("params")) {
			if(!msg.method) {
				// Cannot decode: we don't know what the method is yet
				debug_w("NO METHOD FOUND YET");
				return true;
			}
			msg.kind = Message::Kind::params;
			auto& params = info[0];
			params = root.findObject("params", 6);
			if(!params) {
				debug_w("[JRPC] MISSING params");
				return false;
			}

			auto& paramSelection = info[1];
			paramSelection = params.findObject(msg.method.c_str(), msg.method.length());
			if(!paramSelection) {
				debug_e("[JRPC] MISSING %s", msg.method.c_str());
				return false;
			}

			return true;
		}

		if(element.keyIs("result")) {
			msg.kind = Message::Kind::result;
			/*
				TODO: Result content is dependent upon the method.
				It could be a simple value, or an object.
				The method *may* be recorded in the database but that won't work if
				multiple requests have been sent out: we'd need to track id, etc. separately.
			*/
			if(element.isContainer()) {
				auto& result = info[1];
				result = root.findObject("result", 6);
				if(result) {
					return true;
				}
			} else {
				auto prop = root.findProperty("result", 6);
				if(prop && prop.setJsonValue(element.value, element.valueLength)) {
					return true;
				}
			}
			debug_e("[JRPC] MISSING result");
			return false;
		}

		if(element.keyIs("error")) {
			msg.kind = Message::Kind::error;
			auto& error = info[1];
			error = root.findObject("error", 5);
			if(!error) {
				debug_e("[JRPC] MISSING error");
				return false;
			}
		}

		return true;
	}

	ConfigDB::Object& root;
	Message& msg;
};

Message importMessage(ConfigDB::Database& db, const String& jsonString)
{
	auto store = db.openStoreForUpdate(0);
	if(!store) {
		return {};
	}
	store->resetToDefaults();

	Message msg;
	RpcStream stream(*store, msg);
	stream.print(jsonString);

	if(msg.method && msg.kind == Message::Kind::none) {
		// Second pass
		stream.reset();
		stream.print(jsonString);
	}

	auto status = stream.getStatus();

	if(!status) {
		debug_e("importMessage failed: %s", toString(status).c_str());
		return {};
	}

	return msg;
}

bool exportMessage(ConfigDB::Database& db, int id, Print& out)
{
	auto store = db.openStore(0);
	if(!store) {
		return {};
	}

	auto obj = store->getObject(0);
	if(!obj) {
		return false;
	}

	out << "{" << endl << "\"jsonrpc\": \"2.0\"," << endl << "\"id\": " << id << "," << endl;

	auto& root = reinterpret_cast<const ConfigDB::Union&>(*store);
	switch(root.getTag()) {
	case 0: {
		auto params = static_cast<const ConfigDB::Union&>(obj);
		out << "\"method\": \"" << params.getTagString() << "\"," << endl
			<< "\"params\": " << params.getObject(0) << endl;
		break;
	}
	case 1:
		out << "\"result\": ";
		if(obj.typeIs(ConfigDB::ObjectType::Union)) {
			out << obj.getObject(0) << endl;
		} else {
			out << obj << endl;
		}
		break;
	case 2:
		out << "\"error\": " << obj << endl;
		break;
	}

	out << "}" << endl;

	return true;
}

} // namespace JsonRPC
