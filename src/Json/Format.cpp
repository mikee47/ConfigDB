/**
 * ConfigDB/Json/Format.cpp
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

#include <ConfigDB/Json/Format.h>
#include <ConfigDB/Pointer.h>
#include "ReadStream.h"
#include "WriteStream.h"
#include <debug_progmem.h>

namespace ConfigDB::Json
{
Format format;

std::unique_ptr<ExportStream> Format::createExportStream(const PointerContext& ctx, const ExportOptions& options) const
{
	if(auto prop = ctx.getProperty()) {
		return std::make_unique<PropertyStream>(prop.getJsonValue());
	}

	if(auto obj = ctx.getObject()) {
		// TODO: This won't work: ReadStream requires full context
		StoreRef store = ctx.getStore();
		return std::make_unique<ReadStream>(store, obj, options);
	}

	if(auto db = ctx.getDatabase()) {
		return std::make_unique<ReadStream>(*db, options);
	}

	return nullptr;
}

size_t Format::exportToStream(const PointerContext& ctx, Print& output, const ExportOptions& options) const
{
	return ReadStream::print(ctx, output, options);
}

std::unique_ptr<ImportStream> Format::createImportStream(Database& db) const
{
	return std::make_unique<WriteStream>(db);
}

std::unique_ptr<ImportStream> Format::createImportStream(StoreUpdateRef& store, Object& object) const
{
	return std::make_unique<WriteStream>(store, object);
}

Status Format::importFromStream(Object& object, Stream& source) const
{
	auto status = WriteStream::parse(object, source);
	if(!status) {
		debug_w("JSON load '%s': %s", object.getName().c_str(), toString(status).c_str());
	}
	return status;
}

Status Format::importFromStream(Database& database, Stream& source) const
{
	auto status = WriteStream::parse(database, source);
	if(!status) {
		debug_w("JSON load '%s': %s", database.getName().c_str(), toString(status).c_str());
	}
	return status;
}

} // namespace ConfigDB::Json
