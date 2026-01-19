/****
 * ConfigDB/PropertyInfo.h
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
#include <Data/Range.h>
#include "Number.h"
#include <FlashString/Array.hpp>
#include <FlashString/Vector.hpp>

/**
 * @brief Property types with storage size
 */
#define CONFIGDB_PROPERTY_TYPE_MAP(XX)                                                                                 \
	XX(Boolean, 1)                                                                                                     \
	XX(Int8, 1)                                                                                                        \
	XX(Int16, 2)                                                                                                       \
	XX(Int32, 4)                                                                                                       \
	XX(Int64, 8)                                                                                                       \
	XX(Enum, 1)                                                                                                        \
	XX(UInt8, 1)                                                                                                       \
	XX(UInt16, 2)                                                                                                      \
	XX(UInt32, 4)                                                                                                      \
	XX(UInt64, 8)                                                                                                      \
	XX(Number, 4)                                                                                                      \
	XX(String, sizeof(StringId))                                                                                       \
	XX(Object, sizeof(ObjectInfo*))                                                                                    \
	XX(Alias, 0)

namespace ConfigDB
{
DEFINE_FSTR_LOCAL(fstr_empty, "")

enum class PropertyType : uint32_t {
#define XX(name, ...) name,
	CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
};

/**
 * @brief Defines contained string data using index into string pool
 */
using StringId = uint16_t;

struct ObjectInfo;

inline uint8_t getPropertySize(PropertyType type)
{
	switch(type) {
#define XX(tag, size)                                                                                                  \
	case PropertyType::tag:                                                                                            \
		return size;
		CONFIGDB_PROPERTY_TYPE_MAP(XX)
#undef XX
	}
	return 0;
}

struct EnumInfo {
	PropertyType type;		 ///< The actual store type for this enum (String, etc.)
	FSTR::ObjectBase values; //< Possible values

	uint8_t getItemSize() const
	{
		return (type == PropertyType::String) ? sizeof(FSTR::String*) : getPropertySize(type);
	}

	unsigned length() const
	{
		return values.length() / getItemSize();
	}

	String getString(uint8_t index) const;
	int find(const char* value, unsigned length) const;

	template <typename T> const FSTR::Array<T>& getArray() const
	{
		return values.as<FSTR::Array<T>>();
	}

	const FSTR::Vector<FSTR::String>& getStrings() const
	{
		return values.as<FSTR::Vector<FSTR::String>>();
	}

	template <typename T> T getValue(uint8_t index) const
	{
		return getArray<T>()[index];
	}
};

/**
 * @brief Clamp an integer to the range of a specific storage type
 * @tparam T Type of integer value is to be clamped to
 */
template <typename T, typename U> constexpr T clamp(U value)
{
	using L = std::numeric_limits<T>;
	return TRange<U>{L::min(), L::max()}.clip(value);
}

/**
 * @brief Property metadata
 */
struct PropertyInfo {
	template <typename T, typename U> struct RangeTemplate {
		T minimum;
		T maximum;

		U clip(U value) const
		{
			return TRange(U(minimum), U(maximum)).clip(value);
		}

		static U clip(const RangeTemplate<T, U>* range, U value)
		{
			return range ? range->clip(value) : clamp<U>(value);
		}
	};
	using RangeNumber = RangeTemplate<const_number_t, number_t>;
	using RangeInt8 = RangeTemplate<int32_t, int64_t>;
	using RangeInt16 = RangeTemplate<int32_t, int64_t>;
	using RangeInt32 = RangeTemplate<int32_t, int64_t>;
	using RangeInt64 = RangeTemplate<int64_t, int64_t>;
	using RangeUInt8 = RangeTemplate<uint32_t, int64_t>;
	using RangeUInt16 = RangeTemplate<uint32_t, int64_t>;
	using RangeUInt32 = RangeTemplate<uint32_t, int64_t>;
	using RangeUInt64 = RangeTemplate<uint64_t, int64_t>;

	// Variant property information depends on type
	union Variant {
		const FlashString* defaultString;
		const ObjectInfo* object;
		const EnumInfo* enuminfo;
		const RangeNumber* number;
		const RangeInt8* int8;
		const RangeInt16* int16;
		const RangeInt32* int32;
		const RangeInt64* int64;
		const RangeUInt8* uint8;
		const RangeUInt16* uint16;
		const RangeUInt32* uint32;
		const RangeUInt64* uint64;
	};

	PropertyType type;
	const FlashString& name;
	uint32_t offset; ///< Location of property data in parent object, OR Alias property index
	Variant variant;

	static const PropertyInfo empty;

	explicit operator bool() const
	{
		return this != &empty;
	}

	bool isStringType() const
	{
		if(type == PropertyType::String) {
			return true;
		}
		if(type == PropertyType::Enum && variant.enuminfo->type == PropertyType::String) {
			return true;
		}
		return false;
	}

	/**
	 * @brief Get number of bytes required to store this property value within a structure
	 */
	uint8_t getSize() const
	{
		return getPropertySize(type);
	}

	/**
	 * @brief Find named object information
	 */
	int findObject(const char* name, unsigned length) const;

	/**
	 * @brief Find named object information
	 */
	int findProperty(const char* name, unsigned length) const;

	/**
	 * @brief Get child object by index
	 */
	const PropertyInfo& getObject(unsigned index) const;
};

} // namespace ConfigDB

String toString(ConfigDB::PropertyType type);
