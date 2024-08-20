/**
 * ConfigDB/Reader.h
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
#include <Data/Stream/DataSourceStream.h>

namespace ConfigDB
{
/**
 * @brief Abstract base class for serialising data (reading from database)
 *
 * Serialisation requirements:
 * 
 * - Single object (and children) including a Store (e.g. for saving)
 * - Entire database (multiple stores)
 * - To file or as read-only stream passed in http response
 *
 * Some options:
 * 
 * - Inherit from IDataSourceStream, Printable and initialise in constructor
 * - Standard default constructor but add methods to print or createStream as required
 *
 * This sounds better. As such this is just a simple container.
 * It's not a namespace as it can't be virtually constructed.
 */
class Reader
{
public:
	Reader() = default;

	/**
	 * @brief Create a stream to serialise the entire database
	 * This is used for streaming asychronously to a web client, for example in an HttpResponse.
	 */
	virtual std::unique_ptr<IDataSourceStream> createStream(Database& db) const = 0;

	/**
	 * @brief Create a stream to serialised a store
	 * This is used for streaming asychronously to a web client, for example in an HttpResponse.
	 */
	virtual std::unique_ptr<IDataSourceStream> createStream(std::shared_ptr<Store> store) const = 0;

	/**
	 * @brief Print object
	 * @retval size_t Number of characters written
	 */
	virtual size_t saveToStream(const Object& object, Print& output) const = 0;

	/**
	 * @brief Serialise entire database directly to an output stream
	 * @retval size_t Number of bytes written to the stream
	 */
	virtual size_t saveToStream(Database& database, Print& output) = 0;

	/**
	 * @brief Get the standard file extension for the reader implementation
	 */
	virtual String getFileExtension() const = 0;

	/**
	 * @brief Serialise a store directly to a local file
	 */
	bool saveToFile(const Store& store, const String& filename);

	/**
	 * @brief Serialise a store directly to the default file as determined by the database path
	 */
	bool saveToFile(const Store& store);

	/**
	 * @brief Serialise entire database to a file
	 */
	bool saveToFile(Database& database, const String& filename);
};

} // namespace ConfigDB
