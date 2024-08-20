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

	virtual std::unique_ptr<ReadWriteStream> createStream(Database& db) const = 0;

	virtual std::unique_ptr<ReadWriteStream> createStream(std::shared_ptr<Store> store) const = 0;

	/**
	 * @brief Load store into RAM
	 *
	 * Not normally called directly by applications.
	 *
	 * Loading starts with default data loaded from schema, which is then updated during load.
	 * Failure indicates corrupt JSON file, but any readable data is available.
	 *
	 * @note All existing objects are invalidated
	 */
	virtual bool loadFromStream(Store& store, Stream& source) = 0;

	virtual bool loadFromStream(Database& database, Stream& source) = 0;

	virtual String getFileExtension() const = 0;

	bool loadFromFile(Store& store, const String& filename);

	bool loadFromFile(Store& store);

	bool loadFromFile(Database& database, const String& filename);
};

} // namespace ConfigDB
