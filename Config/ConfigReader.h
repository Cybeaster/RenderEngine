#pragma once
#include "Logger.h"
#include "boost/property_tree/json_parser.hpp"
#include "boost/property_tree/ptree.hpp"

class OConfigReader
{
public:
	OConfigReader(const string& ConfigPath)
	    : FileName(ConfigPath) {}

	void LoadConfig(const string& FileName)
	{
		if (!bIsLoaded)
		{
			read_json(FileName, PTree);
			bIsLoaded = true;
		}
	}

	template<typename T>
	T Get(const std::string& Key) const
	{
		if (PTree.get_child_optional(Key) == boost::none)
		{
			LOG(Debug, Error, "Key not found: {}", TO_STRING(Key));
		}
		return PTree.get<T>(Key);
	}

	template<typename T>
	T GetChild(const std::string& Key, const std::string& ChildKey) const
	{
		if (PTree.get_child_optional(Key) == boost::none)
		{
			LOG(Debug, Error, "Key not found: {}", TO_STRING(Key));
		}

		if (PTree.get_child(Key).get_child_optional(ChildKey) == boost::none)
		{
			LOG(Debug, Error, "Child key not found: {}", TO_STRING(ChildKey));
		}

		return PTree.get_child(Key).get<T>(ChildKey);
	}

protected:
	string FileName;
	boost::property_tree::ptree PTree;
	bool bIsLoaded = false;
};