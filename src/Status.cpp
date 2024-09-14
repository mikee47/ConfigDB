/**
 * ConfigDB/Status.cpp
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

#include "include/ConfigDB/Status.h"

String toString(ConfigDB::FormatError error)
{
	switch(error) {
#define XX(name)                                                                                                       \
	case ConfigDB::FormatError::name:                                                                                  \
		return F(#name);
		CONFIGDB_FORMAT_ERROR_MAP(XX)
#undef XX
	default:
		return nullptr;
	}
}

namespace ConfigDB
{
String Status::toString() const
{
	String s;

	switch(error) {
#define XX(tag)                                                                                                        \
	case Error::tag:                                                                                                   \
		s = F(#tag);                                                                                                   \
		break;
		CONFIGDB_ERROR_MAP(XX)
#undef XX
	}

	switch(error) {
	case Error::FormatError:
		s += "::";
		s += ::toString(code.formatError);
		break;
	case Error::FileError:
		s += "::";
		s += IFS::Error::toString(code.fileError ?: IFS::Error::WriteFailure);
		break;
	case Error::OK:
	case Error::UpdateConflict:
		break;
	}

	return s;
}

} // namespace ConfigDB
