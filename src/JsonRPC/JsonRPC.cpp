#include <ConfigDB/JsonRPC/JsonRPC.h>
#include <ConfigDB/Database.h>
#include <ConfigDB/JsonRPC/WriteStream.h>

namespace JsonRPC
{
Message importMessage(ConfigDB::Database& db, const String& jsonString, GetRequestTag getRequestTag)
{
	auto store = db.openStoreForUpdate(0);
	if(!store) {
		return {};
	}
	store->resetToDefaults();

	Message msg{};
	WriteStream stream(*store, msg, getRequestTag);
	stream.print(jsonString);

	if(stream.isReparseRequired()) {
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

bool exportMessage(ConfigDB::Database& db, const Message& msg, Print& out)
{
	if(msg.kind == Message::Kind::none) {
		return false;
	}

	auto store = db.openStore(0);
	if(!store) {
		return false;
	}

	out << _F("{\r\n"
			  "\"jsonrpc\": \"2.0\"");

	if(msg.kind != Message::Kind::notification) {
		out << _F(",\r\n"
				  "\"id\": ")
			<< msg.id;
	}

	auto& root = reinterpret_cast<ConfigDB::Union&>(*store);
	auto request = root.getObject(0);
	switch(msg.kind) {
	case Message::Kind::request: {
		out << _F(",\r\n"
				  "\"method\": \"")
			<< root.getTagString() << '"';
		if(auto params = request.findObject("params", 6)) {
			out << ",\r\n" << _F("\"params\": ") << params;
		}
		break;
	}
	case Message::Kind::result:
	case Message::Kind::notification: {
		if(auto result = request.findObject("result", 6)) {
			out << _F(",\r\n"
					  "\"result\": ")
				<< result;
		}
		break;
	}
	case Message::Kind::error: {
		if(auto error = request.findObject("error", 5)) {
			out << _F(",\r\n"
					  "\"error\": ")
				<< error;
		}
		break;
	}
	case Message::Kind::none:
		break;
	}

	out << _F("\r\n}\r\n");

	return true;
}

} // namespace JsonRPC
