/**
 * ConfigDB/Format/Json.h
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

#include "../Format.h"

namespace ConfigDB::Json
{
class Format : public ConfigDB::Format
{
public:
	DEFINE_FSTR_LOCAL(fileExtension, ".json")

	std::unique_ptr<ExportStream> createExportStream(Database& db) const override;
	std::unique_ptr<ExportStream> createExportStream(std::shared_ptr<Store> store, const Object& object) const override;
	size_t exportToStream(const Object& object, Print& output) const override;
	size_t exportToStream(Database& database, Print& output) const override;
	std::unique_ptr<ImportStream> createImportStream(Database& db) const override;
	std::unique_ptr<ImportStream> createImportStream(std::shared_ptr<Store> store, Object& object) const override;
	Status importFromStream(Object& object, Stream& source) const override;
	Status importFromStream(Database& database, Stream& source) const override;

	String getFileExtension() const override
	{
		return fileExtension;
	}

	MimeType getMimeType() const override
	{
		return MimeType::JSON;
	}

	void setPretty(bool pretty)
	{
		this->pretty = pretty;
	}

private:
	bool pretty{false};
};

extern Format format;

} // namespace ConfigDB::Json
