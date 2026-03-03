#pragma once
#include <string>
#include <map>

class ResponseParser
{
public:
    // Auto-detect JSON or URL params format and parse into key-value map
    // All values are stored as strings (numbers converted to string)
    static std::map<std::string, std::string> Parse(const std::string& data);

    static std::string GetValue(const std::map<std::string, std::string>& params,
                                const std::string& key,
                                const std::string& defaultVal = "");

private:
    static std::map<std::string, std::string> ParseJson(const std::string& data);
    static std::map<std::string, std::string> ParseUrlParams(const std::string& data);
};

// Keep old name as alias for compatibility
using UrlParser = ResponseParser;
