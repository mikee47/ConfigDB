#include "JsonRPC.h"
#include <ConfigDB/Database.h>
#include <ConfigDB/Json/Format.h>
#include <JSON/StreamingParser.h>
#include <Data/Stream/LimitedMemoryStream.h>

namespace JsonRPC
{
class Listener : public JSON::Listener
{
public:
	using Element = JSON::Element;

	Listener(Message& msg) : msg(msg)
	{
		msg = Message{};
	}

	bool startElement(const Element& element) override
	{
		if(element.level != 1) {
			//
		} else if(element.keyIs("method")) {
			msg.method = element.as<String>();
		} else if(element.keyIs("id")) {
			msg.id = element.as<int>();
		} else if(element.keyIs("params")) {
			msg.kind = Message::Kind::params;
			msg.isContainer = element.isContainer();
			msg.content.start = pos;
		} else if(element.keyIs("result")) {
			msg.kind = Message::Kind::result;
			msg.isContainer = element.isContainer();
			msg.content.start = pos;
		} else if(element.keyIs("error")) {
			msg.kind = Message::Kind::error;
			msg.isContainer = element.isContainer();
			msg.content.start = pos;
		}
		return true;
	}

	bool endElement(const Element& element) override
	{
		if(element.level != 1) {
			//
		} else if(msg.content.start) {
			msg.content.length = 1 + pos - msg.content.start;
		}
		return true;
	}

	Message& msg;
	const char* pos{};
};

Message importMessage(ConfigDB::Database& db, const String& jsonString)
{
	Message msg;
	Listener listener(msg);
	JSON::StaticStreamingParser<128> parser(&listener);
	listener.pos = jsonString.c_str();
	for(auto c : jsonString) {
		auto status = parser.parse(c);
		if(status > JSON::Status::EndOfDocument) {
			debug_e("JSON parsing error: %s", toString(status).c_str());
			return {};
		}
		++listener.pos;
	}

	auto store = db.openStoreForUpdate(0);
	if(!store) {
		return {};
	}
	store->resetToDefaults();
	ConfigDB::Object obj;
	switch(msg.kind) {
	case Message::Kind::none:
		break;

	case Message::Kind::params:
		if(auto params = store->findObject("params", 6)) {
			obj = params.findObject(msg.method.c_str(), msg.method.length());
		}
		break;

	case Message::Kind::error:
		obj = store->findObject("error", 5);
		break;

	case Message::Kind::result:
		/*
			TODO: Result content is dependent upon the method.
			It could be a simple value, or an object.
			The method *may* be recorded in the database but that won't work if
			multiple requests have been sent out: we'd need to track id, etc. separately.
		*/
		if(msg.isContainer) {
			obj = store->findObject("result", 6);
		} else {
			auto prop = store->findProperty("result", 6);
			prop.setJsonValue(msg.content.start, msg.content.length);
		}
		break;
	}

	if(obj && msg.content) {
		LimitedMemoryStream mem(const_cast<char*>(msg.content.start), msg.content.length, msg.content.length, false);
		obj.importFromStream(ConfigDB::Json::format, mem);
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
