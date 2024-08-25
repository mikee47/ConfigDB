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

#include "Store.h"
#include <Data/Stream/ReadWriteStream.h>

namespace ConfigDB
{
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
	virtual std::unique_ptr<IDataSourceStream> createExportStream(Database& db) const = 0;

	/**
	 * @brief Create a stream to serialize a store
	 * This is used for streaming asychronously to a web client, for example in an HttpResponse.
	 */
	virtual std::unique_ptr<IDataSourceStream> createExportStream(std::shared_ptr<Store> store) const = 0;

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
	virtual std::unique_ptr<ReadWriteStream> createImportStream(Database& db) const = 0;

	/**
	 * @brief Create a stream for de-serialising (writing) into a store
	 * Used when updating a store from a remote web client, for example via HttpRequest
	 */
	virtual std::unique_ptr<ReadWriteStream> createImportStream(std::shared_ptr<Store> store) const = 0;

	/**
	 * @brief De-serialise content from stream into store (RAM)
	 * The store is not saved by this operation.
	 */
	virtual bool importFromStream(Store& store, Stream& source) const = 0;

	/**
	 * @brief De-serialise content from stream into database
	 * Each store is overwritten as it is loadded.
	 * If a store entry is not represented in the data then it is left untouched.
	 */
	virtual bool importFromStream(Database& database, Stream& source) const = 0;

	/**
	 * @brief Get the standard file extension for the reader implementation
	 */
	virtual String getFileExtension() const = 0;

	/**
	 * @brief Get the MIME type for this reader format
	 */
	virtual MimeType getMimeType() const = 0;

	//// Try to get rid of these methods so this is a pure virtual class
	//// This code can go into Database, Object, Store, etc.

	/**
	 * @brief Serialise a store directly to a local file
	 */
	bool exportToFile(const Store& store, const String& filename) const;

	/**
	 * @brief Serialise a store directly to the default file as determined by the database path
	 */
	bool exportToFile(const Store& store) const;

	/**
	 * @brief Serialise entire database to a file
	 */
	bool exportToFile(Database& database, const String& filename) const;

	/**
	 * @brief De-serialise content from file into store
	 * The store is not saved by this operation.
	 */
	bool importFromFile(Store& store, const String& filename) const;

	/**
	 * @brief De-serialise content from default location into store
	 * The store is not saved by this operation.
	 */
	bool importFromFile(Store& store) const;

	/**
	 * @brief De-serialise content from file into database
	 */
	bool importFromFile(Database& database, const String& filename) const;
};

} // namespace ConfigDB
