//Text.cc

#include "sybie/common/Text.hh"

#include <cstdlib>

namespace sybie {
namespace common {

int ParseInt(const std::string& str)
{
    return atoi(str.c_str());
}

double ParseFloat(const std::string& str)
{
    return atof(str.c_str());
}

std::string LowerCase(const std::string& str)
{
    std::string result = str;
    return SetLowerCase(result);
}

std::string UpperCase(const std::string& str)
{
    std::string result = str;
    return SetUpperCase(result);
}

std::string& SetLowerCase(std::string& str)
{
    for (char& c : str)
        c = tolower(c);
    return str;
}

std::string& SetUpperCase(std::string& str)
{
    for (char& c : str)
        c = toupper(c);
    return str;
}

std::string TrimRight(
    const std::string& s,
    const std::string& delimiters)
{
    size_t pos = s.find_last_not_of(delimiters);
    if (pos == std::string::npos)
        return "";
    else
        return s.substr(0, pos + 1);
}

std::string TrimLeft(
    const std::string& s,
    const std::string& delimiters)
{
    size_t pos = s.find_first_not_of(delimiters);
    if (pos == std::string::npos)
        return "";
    else
        return s.substr(pos);
}

std::string Trim(
    const std::string& s,
    const std::string& delimiters)
{
    return TrimLeft(TrimRight(s, delimiters), delimiters);
}

std::string GetDirPath(const std::string& filepath)
{
    std::string::size_type pos = filepath.find_last_of("/\\");
    if (pos == std::string::npos)
        return "";
    else
        return filepath.substr(0, pos + 1);
}

std::string GetFilename(const std::string& filepath)
{
    std::string::size_type pos = filepath.find_last_of("/\\");
    if (pos == std::string::npos)
        return filepath;
    else
        return filepath.substr(pos + 1);
}

bool StartsWith(const std::string& str, const std::string& substr)
{
    if (str.size() < substr.size())
        return false;
    for (size_t i = 0 ; i < substr.size() ; i++)
        if (str[i] != substr[i])
            return false;
    return true;
}

bool EndsWith(const std::string& str, const std::string& substr)
{
    if (str.size() < substr.size())
        return false;
    size_t offset = str.size() - substr.size();
    for (size_t i = 0 ; i < substr.size() ; i++)
        if (str[offset + i] != substr[i])
            return false;
    return true;
}

}  //namespace common
}  //namespace sybie
