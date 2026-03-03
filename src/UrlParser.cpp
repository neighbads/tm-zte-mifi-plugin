#include "UrlParser.h"

std::map<std::string, std::string> ResponseParser::Parse(const std::string& data)
{
    // Trim leading whitespace
    size_t start = data.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return {};

    if (data[start] == '{')
        return ParseJson(data);
    else
        return ParseUrlParams(data);
}

std::map<std::string, std::string> ResponseParser::ParseJson(const std::string& data)
{
    std::map<std::string, std::string> result;

    // Find content between { and }
    size_t start = data.find('{');
    size_t end = data.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start)
        return result;

    std::string content = data.substr(start + 1, end - start - 1);
    size_t pos = 0;

    while (pos < content.size())
    {
        // Skip whitespace and commas
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == ',' ||
               content[pos] == '\t' || content[pos] == '\r' || content[pos] == '\n'))
            pos++;

        if (pos >= content.size())
            break;

        // Parse key (must be quoted string)
        if (content[pos] != '"')
            break;
        pos++; // skip opening quote

        size_t keyEnd = content.find('"', pos);
        if (keyEnd == std::string::npos)
            break;

        std::string key = content.substr(pos, keyEnd - pos);
        pos = keyEnd + 1;

        // Skip whitespace and colon
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == ':'))
            pos++;

        if (pos >= content.size())
            break;

        // Parse value
        std::string value;
        if (content[pos] == '"')
        {
            // String value
            pos++; // skip opening quote
            size_t valEnd = content.find('"', pos);
            if (valEnd == std::string::npos)
                break;
            value = content.substr(pos, valEnd - pos);
            pos = valEnd + 1;
        }
        else
        {
            // Number, boolean, or null
            size_t valEnd = pos;
            while (valEnd < content.size() && content[valEnd] != ',' &&
                   content[valEnd] != '}' && content[valEnd] != ' ' &&
                   content[valEnd] != '\r' && content[valEnd] != '\n')
                valEnd++;
            value = content.substr(pos, valEnd - pos);
            pos = valEnd;
        }

        if (!key.empty())
            result[key] = value;
    }

    return result;
}

std::map<std::string, std::string> ResponseParser::ParseUrlParams(const std::string& data)
{
    std::map<std::string, std::string> result;
    size_t pos = 0;
    while (pos < data.size())
    {
        size_t ampPos = data.find('&', pos);
        if (ampPos == std::string::npos)
            ampPos = data.size();

        std::string pair = data.substr(pos, ampPos - pos);
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);
            result[key] = value;
        }
        pos = ampPos + 1;
    }
    return result;
}

std::string ResponseParser::GetValue(const std::map<std::string, std::string>& params,
                                     const std::string& key,
                                     const std::string& defaultVal)
{
    auto it = params.find(key);
    if (it != params.end())
        return it->second;
    return defaultVal;
}
