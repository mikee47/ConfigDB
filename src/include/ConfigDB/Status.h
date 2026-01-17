/****
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

#include <WString.h>
#include <IFS/Error.h>

#define CONFIGDB_ERROR_MAP(XX)                                                                                         \
	XX(OK)                                                                                                             \
	XX(FormatError)                                                                                                    \
	XX(UpdateConflict)                                                                                                 \
	XX(FileError)

#define CONFIGDB_FORMAT_ERROR_MAP(XX)                                                                                  \
	XX(BadSyntax)                                                                                                      \
	XX(BadType)                                                                                                        \
	XX(BadSelector)                                                                                                    \
	XX(BadIndex)                                                                                                       \
	XX(BadProperty)                                                                                                    \
	XX(NotInSchema)                                                                                                    \
	XX(UpdateConflict)

namespace ConfigDB
{
enum class Error {
#define XX(err) err,
	CONFIGDB_ERROR_MAP(XX)
#undef XX
};

enum class FormatError {
#define XX(err) err,
	CONFIGDB_FORMAT_ERROR_MAP(XX)
#undef XX
};

struct Status {
	struct Code {
		int fileError{};
		FormatError formatError;
	};

	Error error{};
	Code code;

	static Status fileError(int errorCode)
	{
		return Status{Error::FileError, {.fileError = errorCode}};
	}

	static Status formatError(FormatError err)
	{
		return Status{Error::FormatError, {.formatError = err}};
	}

	Status& operator=(FormatError err)
	{
		error = Error::FormatError;
		code.formatError = err;
		return *this;
	}

	explicit operator bool() const
	{
		return error == Error::OK;
	}

	String toString() const;

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

String toString(ConfigDB::FormatError error);
