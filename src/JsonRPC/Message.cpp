#include <ConfigDB/JsonRPC/Message.h>

namespace JsonRPC
{
#define XX(tag) DEFINE_FSTR(FS_##tag, #tag)
STRING_MAP(XX)
#undef XX

} // namespace JsonRPC

String toString(JsonRPC::Message::Kind kind)
{
	using namespace JsonRPC;
	using Kind = Message::Kind;
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
