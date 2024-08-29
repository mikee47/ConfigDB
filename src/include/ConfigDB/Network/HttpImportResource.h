/**
 * ConfigDB/HttpImportResource.h
 *
 * Copyright 2024 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the ConfigDB Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#pragma once

#include "../Database.h"
#include <Network/Http/HttpResource.h>

namespace ConfigDB
{
/**
 * @brief HttpResource handler to support streaming updates to a configuration database
 */
class HttpImportResource : public HttpResource
{
public:
	/**
	 * @brief Callback invoked when a POST request received
	 * @param request The incoming request
	 * @param mimeType Decoded Content-Type field from request
	 * @retval std::unique_ptr<ReadWriteStream> Writeable stream created by calling, for example, `ConfigDB::Database.createExportStream`
	 * Return null if request is to be rejected
	 */
	using OnRequest = Delegate<std::unique_ptr<ImportStream>(HttpRequest& request, MimeType mimeType)>;

	/**
	 * @brief Callback invoked when a POST request has completed
	 * @param request The request previously passed to `onRequest` callback
	 * @param response The response message that will be returned to the client
	 * @param stream The stream created by the `onRequest` call
	 *
	 * Typically this callback will inspect the status of the update operation and return a suitable code or message in the response.
	 */
	using OnComplete = Delegate<void(HttpRequest& request, HttpResponse& response, ImportStream& stream)>;

	/**
	 * @brief Construct a resource handler
	 */
	HttpImportResource(OnRequest onRequest, OnComplete onComplete) : onRequest(onRequest), onComplete(onComplete)
	{
		init();
	}

	HttpImportResource(Database& database, const Format& format)
	{
		init();

		onRequest = [&database, &format](HttpRequest&, MimeType mimeType) -> std::unique_ptr<ImportStream> {
			debug_i("POST REQ");
			if(mimeType != format.getMimeType()) {
				return nullptr;
			}
			return database.createImportStream(format);
		};

		onComplete = [](HttpRequest&, HttpResponse& response, ImportStream& stream) -> void {
			String msg;
			msg += F("Result: ");
			auto status = stream.getStatus();
			msg += toString(status);
			response.sendString(msg);
			debug_i("%s", msg.c_str());
			switch(status.error) {
			case Error::OK:
				break;
			case Error::FormatError:
				response.code = HTTP_STATUS_BAD_REQUEST;
				break;
			case Error::UpdateConflict:
				response.code = HTTP_STATUS_CONFLICT;
				break;
			case Error::FileError:
				response.code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
				break;
			}
		};
	}

private:
	void init()
	{
		onHeadersComplete = [this](HttpServerConnection&, HttpRequest& request, HttpResponse& response) -> int {
			if(request.args) {
				response.code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
				return 0;
			}

			if(request.method != HTTP_POST) {
				response.code = HTTP_STATUS_BAD_REQUEST;
				return 0;
			}

			String contentType = request.headers[HTTP_HEADER_CONTENT_TYPE];
			auto stream = this->onRequest(request, ContentType::fromString(contentType));
			if(!stream) {
				response.code = HTTP_STATUS_BAD_REQUEST;
				return 0;
			}

			request.args = stream.release();
			return 0;
		};

		onBody = [this](HttpServerConnection&, HttpRequest& request, const char* at, int length) -> int {
			auto stream = static_cast<ImportStream*>(request.args);
			if(!stream) {
				return 0;
			}
			return stream->write(at, length) == size_t(length) ? 0 : 1;
		};

		onRequestComplete = [this](HttpServerConnection&, HttpRequest& request, HttpResponse& response) -> int {
			auto stream = static_cast<ImportStream*>(request.args);
			if(stream) {
				this->onComplete(request, response, *stream);
				delete stream;
				request.args = nullptr;
			}
			return 0;
		};
	}

	OnRequest onRequest;
	OnComplete onComplete;
};

} // namespace ConfigDB
