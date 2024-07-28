#pragma once

#include <WString.h>
#include <Data/CString.h>

namespace ConfigDB
{
enum class Mode {
	readonly,
	readwrite,
};

class Config
{
public:
	Config(const String& path) : path(path.c_str())
	{
	}

	String getPath() const
	{
		return path.c_str();
	}

private:
	CString path;
};

/**
 * @brief Non-virtual implementation template
 */
class Store
{
public:
	// Store(const String& name, Mode mode)
	// {
	// }

	// bool commit()
	// {
	// 	return false;
	// }

	// template <typename T> bool setValue(const String& key, const T& value)
	// {
	// 	return false;
	// }

	// template <typename T> bool getValue(const String& key)
	// {
	// 	return false;
	// }
};


} // namespace ConfigDB
