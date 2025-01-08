/**
 * ConfigDB/Json/Printer.h
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

#include <ConfigDB/Object.h>
#include <JSON/StreamingParser.h>

namespace ConfigDB::Json
{
/**
 * @brief Class to serialise objects in stages to minimise RAM usage
 */
class Printer
{
public:
	Printer() = default;

	Printer(Print& p, const Object& object, bool pretty, RootStyle style);

	void reset();

	void setRootStyle(RootStyle style);

	explicit operator bool() const
	{
		return p;
	}

	/**
	 * @brief Print some data
	 * @retval size_t number of characters output
	 */
	size_t operator()();

	/**
	 * @brief Print a newline, if appropriate for mode
	 */
	size_t newline()
	{
		return pretty ? p->println() : 0;
	}

	/**
	 * @brief Determine if everything has been printed
	 */
	bool isDone() const
	{
		return nesting == 255;
	}

private:
	Print* p{};
	Object objects[JSON::StreamingParser::maxNesting];
	RootStyle rootStyle{};
	uint8_t nesting{};
	bool pretty{};
};

} // namespace ConfigDB::Json
