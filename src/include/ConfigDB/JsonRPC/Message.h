#pragma once

#include <Print.h>
#include <ConfigDB/Object.h>

namespace JsonRPC
{
#define STRING_MAP(XX)                                                                                                 \
	XX(method)                                                                                                         \
	XX(none)                                                                                                           \
	XX(params)                                                                                                         \
	XX(request)                                                                                                        \
	XX(result)                                                                                                         \
	XX(notification)                                                                                                   \
	XX(error)

#define XX(tag) DECLARE_FSTR(FS_##tag)
STRING_MAP(XX)
#undef XX

/** 
 * @brief RPC metadata
 */
struct Message {
	enum class Kind {
		none,		  ///< No valid message
		request,	  ///< May have params (optional)
		notification, ///< Same as a result but with no ID
		result,		  ///< Successful response to a request
		error,		  ///< An error response
	};

	int id{0};
	Kind kind{};
};

} // namespace JsonRPC

String toString(JsonRPC::Message::Kind kind);
