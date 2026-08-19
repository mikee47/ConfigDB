#include <ConfigDB/JsonRPC/JsonRPC.h>
#include <ConfigDB/Database.h>
#include <ConfigDB/Json/WriteStream.h>
#include <JSON/StreamingParser.h>

namespace
{
#define STRING_MAP(XX)                                                                                                 \
	XX(method)                                                                                                         \
	XX(none)                                                                                                           \
	XX(params)                                                                                                         \
	XX(request)                                                                                                        \
	XX(result)                                                                                                         \
	XX(notification)                                                                                                   \
	XX(error)

#define XX(tag) DEFINE_FSTR_LOCAL(FS_##tag, #tag)
STRING_MAP(XX)
#undef XX

} // namespace

String toString(JsonRPC::Message::Kind kind)
{
	using Kind = JsonRPC::Message::Kind;
	switch(kind) {
	case Kind::none:
		return FS_none;
	case Kind::request:
		return FS_request;
	case Kind::notification:
		return FS_notification;
	case Kind::result:
		return FS_result;
	case Kind::error:
		return FS_error;
	}
	return nullptr;
}

namespace JsonRPC
{
class RpcStream : public ConfigDB::Json::WriteStream
{
public:
	using Element = JSON::Element;

	RpcStream(ConfigDB::Object& obj, Message& msg, GetRequestTag& getRequestTag)
		: WriteStream(obj), root(static_cast<ConfigDB::Union&>(obj)), msg(msg), getRequestTag(getRequestTag)
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

		if(element.keyIs(FS_method)) {
			auto& request = info[0];
			request = root.findObject(element.value, element.valueLength);
			if(!request) {
				debug_w("[JRPC] Missing %s", element.value);
				return false;
			}

			auto& params = info[1];
			String tag(FS_params);
			params = request.findObject(tag.c_str(), tag.length());
			if(!params) {
				debug_e("[JRPC] Missing %s/%s", element.value, tag.c_str());
				return false;
			}

			haveMethod = true;
			return true;
		}

		if(element.keyIs("id")) {
			msg.id = element.as<int>();
			haveId = true;
			return true;
		}

		if(element.keyIs(FS_params)) {
			if(!haveMethod) {
				// Cannot decode: we need id to determine request type
				repeatParse = true;
				return true;
			}

			msg.kind = haveId ? Message::Kind::request : Message::Kind::notification;
			return true;
		}

		if(element.keyIs(FS_result)) {
			if(!haveId) {
				// Cannot decode: we need id to determine request type
				repeatParse = true;
				return true;
			}

			int tag = getRequestTag(msg.id);
			if(tag < 0) {
				debug_e("[JRPC] Unknown ID %d", msg.id);
				return false;
			}

			root.setTag(tag);
			auto& request = info[0];
			request = root.getObject(0);

			// Result could be a simple value, or an object
			msg.kind = Message::Kind::result;
			if(element.isContainer()) {
				auto& result = info[1];
				result = request.findObject(element.key, element.keyLength);
				if(!result) {
					debug_e("[JRPC] Missing %s/%s", root.getTagString().c_str(), element.key);
					return false;
				}
				return true;
			} else {
				auto prop = root.findProperty(element.key, element.keyLength);
				if(prop && prop.setJsonValue(element.value, element.valueLength)) {
					return true;
				}
			}
			debug_e("[JRPC] Missing result");
			return false;
		}

		if(element.keyIs(FS_error)) {
			if(!haveId) {
				// Cannot decode: we need id to determine request type
				repeatParse = true;
				return true;
			}

			int tag = getRequestTag(msg.id);
			if(tag < 0) {
				debug_e("[JRPC] Unknown ID  %d", msg.id);
				return false;
			}

			root.setTag(tag);
			auto& request = info[0];
			request = root.getObject(0);

			msg.kind = Message::Kind::error;
			auto& error = info[1];
			error = request.findObject(element.key, element.keyLength);
			if(!error) {
				debug_e("[JRPC] Missing %s/%s", root.getTagString().c_str(), element.key);
				return false;
			}
		}

		return true;
	}

	ConfigDB::Union& root;
	Message& msg;
	GetRequestTag getRequestTag;
	bool haveMethod{false};
	bool haveId{false};
	bool repeatParse{false};
};

Message importMessage(ConfigDB::Database& db, const String& jsonString, GetRequestTag getRequestTag)
{
	auto store = db.openStoreForUpdate(0);
	if(!store) {
		return {};
	}
	store->resetToDefaults();

	Message msg{};
	RpcStream stream(*store, msg, getRequestTag);
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
