/****
 * ConfigDB/Accessor.h
 *
 * Copyright 2026 mikee47 <mike@sillyhouse.net>
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

#include "Union.h"

namespace ConfigDB
{
/**
 * @brief Support read/write access to union properties
 */
class Accessor
{
public:
	/**
	 * @brief Construct an accessor
	 * @param obj The Union instance being accessed
	 * @param index Property index
	 */
	Accessor(Union& obj, uint8_t index) : obj(obj), index(index)
	{
	}

protected:
	void get(uint8_t& value) const
	{
		value = obj.getPropertyData(index)->uint8;
	}

	void get(uint16_t& value) const
	{
		value = obj.getPropertyData(index)->uint16;
	}

	void get(uint32_t& value) const
	{
		value = obj.getPropertyData(index)->uint32;
	}

	void get(int8_t& value) const
	{
		value = obj.getPropertyData(index)->int8;
	}

	void get(int16_t& value) const
	{
		value = obj.getPropertyData(index)->int16;
	}

	void get(int32_t& value) const
	{
		value = obj.getPropertyData(index)->int32;
	}

	void get(int64_t& value) const
	{
		value = obj.getPropertyData(index)->int64;
	}

	void get(bool& value) const
	{
		value = obj.getPropertyData(index)->boolean;
	}

	void get(Number& value) const
	{
		value = obj.getPropertyData(index)->number;
	}

	void get(String& value) const
	{
		value = obj.getPropertyString(index);
	}

	template <typename T> void set(const T& value)
	{
		obj.setPropertyValue(index, value);
	}

	/**
	 * @brief Verify that the union tag hasn't been changed
	 */
	bool checkTag() const
	{
		auto tag = obj.typeinfo().objectCount + index;
		if(obj.getTag() == tag) {
			return true;
		}
		assert(false);
		return false;
	}

	Union& obj;
	uint8_t index;
};

/**
 * @brief Support read/write access to union properties by code generator
 * @tparam BaseType Fundamental type of property (e.g. uint8_t, String)
 * @tparam Value Actual type (e.g. enum, user-defined type)
 */
template <typename BaseType, typename Value> class AccessorTemplate : public Accessor
{
public:
	using Accessor::Accessor;

	/**
	 * @brief Implicit cast to read the property value
	 */
	operator Value() const
	{
		if(!checkTag()) {
			return {};
		}
		BaseType value;
		get(value);
		return Value(value);
	}

	/**
	 * @brief Read the property value
	 */
	Value operator*() const
	{
		return operator Value();
	}

	/**
	 * @brief Write the property value via assignment
	 */
	Accessor& operator=(const Value& value)
	{
		if(checkTag()) {
			set(BaseType(value));
		}
		return *this;
	}
};

} // namespace ConfigDB
