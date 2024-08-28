/**
 * ConfigDB/Status.h
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

#include <IFS/Error.h>

namespace ConfigDB
{
enum class Result {
	ok,
	formatError,
	updateConflict,
	fileError,
};

struct Status {
	Result result{};
	int fileError{};

	explicit operator bool() const
	{
		return result == Result::ok;
	}

	String toString() const
	{
		switch(result) {
		case Result::ok:
			return F("OK");
		case Result::formatError:
			return F("Format Error");
		case Result::updateConflict:
			return F("Update Conflict");
		case Result::fileError:
			return IFS::Error::toString(fileError ?: IFS::Error::WriteFailure);
		default:
			return nullptr;
		}
	}

	size_t printTo(Print& p) const
	{
		return p.print(toString());
	}
};

} // namespace ConfigDB

inline String toString(ConfigDB::Status status)
{
	return status.toString();
}
