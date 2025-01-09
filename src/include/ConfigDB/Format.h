/**
 * ConfigDB/Format.h
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

#include <Data/Stream/ReadWriteStream.h>
#include "Status.h"
#include "StoreRef.h"

namespace ConfigDB
{
class Database;
class Store;
class Object;

class ImportStream : public ReadWriteStream
{
public:
	virtual Status getStatus() const = 0;
};

/**
 * @brief Style options for output of root item in export stream
 */
enum class RootStyle {
	content, ///< Show content only 13,28,39,40
	braces,  ///< Add braces [13,28,39,40]
	name,	///< Add name "int_array":[13,28,39,40]
	object,  ///< Add outer braces {"int_array":[13,28,39,40]}
};

/**
 * @brief Options for streaming object output
 */
struct ExportOptions {
	/**
	 * @brief Applicable style for root object output
	 */
	RootStyle rootStyle{};

	/**
	 * @brief Optional override for root object name
	 *
	 * Useful where object is un-named or the name is to change.
	 * Applicable to `name` and `object` root styles.
	 */
	String rootName;

	/**
	 * @brief Set compact (default) or prettified output.
	 */
	bool pretty{false};
};

/**
 * @brief Interface for formatted export stream
 */
class ExportStream : public IDataSourceStream
{
public:
	using Options = ExportOptions;

	virtual Status getStatus() const = 0;

	/**
	 * @brief Get currently active export options
	 */
	virtual Options getOptions() const = 0;

	/**
	 * @brief Set export options
	 * @param options New options
	 */
	virtual void setOptions(const Options& options) = 0;
};

/**
 * @brief Abstract base class wrapping support for a specific storage format, such as JSON
 */
class Format
{
public:
	/**
	 * @brief Create a stream to serialize the entire database
	 * This is used for streaming asychronously to a web client, for example in an HttpResponse.
	 */
	virtual std::unique_ptr<ExportStream> createExportStream(Database& db) const = 0;

	/**
	 * @brief Create a stream to serialize an Object
	 * @param store Shared pointer to store
	 * @param object Object to start streaming from
	 *
	 * Used for streaming asychronously to a web client, for example in an HttpResponse.
	 */
	virtual std::unique_ptr<ExportStream> createExportStream(StoreRef store, const Object& object) const = 0;

	/**
	 * @brief Print object
	 * @retval size_t Number of characters written
	 */
	virtual size_t exportToStream(const Object& object, Print& output) const = 0;

	/**
	 * @brief Serialise entire database directly to an output stream
	 * @retval size_t Number of bytes written to the stream
	 */
	virtual size_t exportToStream(Database& database, Print& output) const = 0;

	/**
	 * @brief Create a stream for de-serialising (writing) into the database
	 * Used when updating a database from a remote web client, for example via HttpRequest.
	 */
	virtual std::unique_ptr<ImportStream> createImportStream(Database& db) const = 0;

	/**
	 * @brief Create a stream for de-serialising (writing) into a store
	 * @param store Shared pointer to store with write access
	 * @param object Object to start streaming to
	 *
	 * Used when updating a store from a remote web client, for example via HttpRequest
	 */
	virtual std::unique_ptr<ImportStream> createImportStream(StoreUpdateRef& store, Object& object) const = 0;

	/**
	 * @brief De-serialise content from stream into object (RAM)
	 */
	virtual Status importFromStream(Object& object, Stream& source) const = 0;

	/**
	 * @brief De-serialise content from stream into database
	 * Each store is overwritten as it is loadded.
	 * If a store entry is not represented in the data then it is left untouched.
	 */
	virtual Status importFromStream(Database& database, Stream& source) const = 0;

	/**
	 * @brief Get the standard file extension for the reader implementation
	 */
	virtual String getFileExtension() const = 0;

	/**
	 * @brief Get the MIME type for this reader format
	 */
	virtual MimeType getMimeType() const = 0;
};

} // namespace ConfigDB
