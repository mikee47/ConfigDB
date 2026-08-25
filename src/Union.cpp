/****
 * ConfigDB/Union.cpp
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

#include "include/ConfigDB/Union.h"

namespace ConfigDB
{
const PropertyData* Union::getDefaultPropertyData(unsigned index) const
{
	auto& ti = typeinfo();
	auto ptr = static_cast<const uint8_t*>(ti.defaultData);
	if(!ptr || index >= ti.propertyCount) {
		assert(false);
		return nullptr;
	}
	if(!ptr) {
		assert(false);
		return nullptr;
	}
	auto prop = ti.propinfo;
	ptr += prop->offset;
	for(unsigned n = ti.objectCount; n--; ++prop) {
		ptr += prop->variant.object->dataSize;
	}
	for(; index--; ++prop) {
		ptr += prop->getSize();
	}
	return reinterpret_cast<const PropertyData*>(ptr);
}

void Union::setTag(Tag tag)
{
	if(!writeCheck()) {
		return;
	}
	auto& ti = typeinfo();
	if(tag >= ti.objectCount + ti.propertyCount) {
		assert(false);
		return;
	}
	auto ptr = static_cast<uint8_t*>(getDataPtr());
	if(tagIsObject(*ptr)) {
		disposeArrays();
	}
	memset(ptr, 0, ti.dataSize);
	*ptr = tag;
	if(tag < ti.objectCount) {
		Object(*this, tag).clear();
	} else {
		unsigned index = tag - ti.objectCount;
		auto& prop = ti.getProperty(index);
		ptr += prop.offset;
		auto defaultData = getDefaultPropertyData(index);
		memcpy_P(ptr, defaultData, prop.getSize());
	}
}

} // namespace ConfigDB
