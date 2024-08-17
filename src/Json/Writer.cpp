/**
 * ConfigDB/Json/Writer.cpp
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

#include <ConfigDB/Json/Writer.h>
#include <JSON/StreamingParser.h>
#include <debug_progmem.h>

namespace ConfigDB::Json
{
Writer writer;

class ConfigListener : public JSON::Listener
{
public:
	using Element = JSON::Element;

	ConfigListener(Store& store) : store(store)
	{
	}

	bool startElement(const Element& element) override
	{
		// debug_i("%s %u %s: '%s' = %u / %s", __FUNCTION__, element.level, toString(element.type).c_str(), element.key,
		// 		element.valueLength, element.value);

		if(element.level == 0) {
			info[0] = Object(store.typeinfo(), store);
			return true;
		}

		auto& parent = info[element.level - 1];
		if(!parent) {
			return true;
		}

		if(element.isContainer()) {
			Object obj;
			if(parent.typeinfo().type == ObjectType::ObjectArray) {
				obj = static_cast<ObjectArray&>(parent).addItem();
			} else {
				obj = parent.findObject(element.key, element.keyLength);
				if(!obj) {
					debug_w("[JSON] Object '%s' not in schema", element.key);
				}
			}
			info[element.level] = obj;
			return true;
		}

		if(parent.typeinfo().type == ObjectType::Array) {
			auto& array = static_cast<Array&>(parent);
			auto propdata = store.parseString(array.getItemType(), element.value, element.valueLength);
			array.addItem(&propdata);
			return true;
		}
		auto prop = parent.findProperty(element.key, element.keyLength);
		if(!prop) {
			debug_w("[JSON] Property '%s' not in schema", element.key);
			return true;
		}
		prop.setJsonValue(element.value, element.valueLength);
		return true;
	}

	bool endElement(const Element&) override
	{
		// Continue parsing
		return true;
	}

private:
	Store& store;
	Object info[JSON::StreamingParser::maxNesting]{};
};

bool Writer::loadFromStream(Store& store, Stream& source)
{
	store.clear();
	ConfigListener listener(store);
	JSON::StaticStreamingParser<1024> parser(&listener);
	auto status = parser.parse(source);

	if(status == JSON::Status::EndOfDocument) {
		return true;
	}

	debug_w("JSON load '%s': %s", store.getName().c_str(), toString(status).c_str());
	return false;
}

std::unique_ptr<ReadWriteStream> Writer::createStream(Database& db) const
{
	return nullptr;
}

std::unique_ptr<ReadWriteStream> Writer::createStream(std::shared_ptr<Store> store) const
{
	return nullptr;
}

} // namespace ConfigDB::Json
