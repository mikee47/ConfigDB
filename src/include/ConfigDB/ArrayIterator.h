/****
 * ConfigDB/ArrayIterator.h
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

namespace ConfigDB
{
/**
 * @brief Array iterator support
 */
template <class ArrayType, typename ItemType> class ArrayIterator
{
public:
	using const_iterator = ArrayIterator;

	ArrayIterator() = default;
	ArrayIterator(const ArrayIterator&) = default;

	ArrayIterator(ArrayType& array, unsigned index) : array(&array), mIndex(index), mCount(array.getItemCount())
	{
	}

	operator bool() const
	{
		return array && (mIndex < mCount);
	}

	bool operator==(const ArrayIterator& rhs) const
	{
		return array == rhs.array && mIndex == rhs.mIndex;
	}

	bool operator!=(const ArrayIterator& rhs) const
	{
		return !operator==(rhs);
	}

	const ItemType operator*() const
	{
		if(!array) {
			return {};
		}
		return (*array)[mIndex];
	}

	ItemType operator*()
	{
		if(!array) {
			abort();
		}
		return (*array)[mIndex];
	}

	unsigned index() const
	{
		return mIndex;
	}

	ArrayIterator& operator++()
	{
		++mIndex;
		return *this;
	}

	ArrayIterator operator++(int)
	{
		ArrayIterator tmp(*this);
		++mIndex;
		return tmp;
	}

private:
	ArrayType* array{};
	unsigned mIndex{0};
	const unsigned mCount{0};
};

} // namespace ConfigDB
