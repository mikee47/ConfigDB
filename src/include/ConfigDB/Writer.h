/**
 * ConfigDB/Writer.h
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
#include <Delegate.h>

namespace ConfigDB
{
/**
 * @brief Abstract base class for de-serialising data (writing to database)
 *
 * Need to de-serialise from full stream (all stores) or just one store (loading).
 */
class Writer
{
public:
	Writer() = default;

	/**
	 * @brief Create a stream for de-serialising (writing) into the database
	 * Used when updating a database from a remote web client, for example via HttpRequest.
	 */
	virtual std::unique_ptr<ReadWriteStream> createStream(Database& db) const = 0;

	/**
	 * @brief Create a stream for de-serialising (writing) into a store
	 * Used when updating a store from a remote web client, for example via HttpRequest
	 */
	virtual std::unique_ptr<ReadWriteStream> createStream(std::shared_ptr<Store> store) const = 0;

	/**
	 * @brief De-serialise content from stream into store (RAM)
	 * The store is not saved by this operation.
	 *
	 * Not normally called directly by applications.
	 *
	 * Loading starts with default data loaded from schema, which is then updated during load.
	 * Failure indicates corrupt JSON file, but any readable data is available.
	 *
	 * @note All existing objects are invalidated
	 */
	virtual bool loadFromStream(Store& store, Stream& source) = 0;

	/**
	 * @brief De-serialise content from stream into database
	 * Each store is overwritten as it is loadded.
	 * If a store entry is not represented in the data then it is left untouched.
	 */
	virtual bool loadFromStream(Database& database, Stream& source) = 0;

	/**
	 * @brief Get the standard file extension for the writer implementation
	 */
	virtual String getFileExtension() const = 0;

	/**
	 * @brief De-serialise content from file into store
	 * The store is not saved by this operation.
	 */
	bool loadFromFile(Store& store, const String& filename);

	/**
	 * @brief De-serialise content from default location into store
	 * The store is not saved by this operation.
	 */
	bool loadFromFile(Store& store);

	/**
	 * @brief De-serialise content from file into database
	 */
	bool loadFromFile(Database& database, const String& filename);
};

} // namespace ConfigDB
