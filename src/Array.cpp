/**
 * ConfigDB/Array.cpp
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

#include "include/ConfigDB/Array.h"

namespace ConfigDB
{
void Array::initDefaults()
{
	auto& info = typeinfo();
	if(!info.defaultData) {
		return;
	}

	auto& item = getItemType();
	if(item.type == PropertyType::String) {
		auto& strings = *static_cast<const FSTR::Vector<FSTR::String>*>(info.defaultData);
		for(auto& s : strings) {
			auto id = getStringId(s);
			addItem(&id);
		}
		return;
	}

	auto& items = *static_cast<const FSTR::ObjectBase*>(info.defaultData);
	auto itemSize = item.getSize();
	for(unsigned offset = 0; offset < items.length(); offset += itemSize) {
		PropertyData buf{};
		items.read(offset, &buf, itemSize);
		addItem(&buf);
	}
}

} // namespace ConfigDB
