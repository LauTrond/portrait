//sybie/commmon/Text.hh

#ifndef INCLUDE_SYBIE_COMMON_TEXT_HH
#define INCLUDE_SYBIE_COMMON_TEXT_HH

#include <string>
#include <sstream>

namespace sybie {
namespace common {

int ParseInt(const std::string& str);
double ParseFloat(const std::string& str);

std::string LowerCase(const std::string& str);
std::string UpperCase(const std::string& str);
std::string& SetLowerCase(std::string& str);
std::string& SetUpperCase(std::string& str);

std::string TrimRight(
    const std::string& s,
    const std::string& delimiters = " \f\n\r\t\v");

std::string TrimLeft(
    const std::string& s,
    const std::string& delimiters = " \f\n\r\t\v");

std::string Trim(
    const std::string& s,
    const std::string& delimiters = " \f\n\r\t\v");

template<class T>
T Parse(const std::string& str)
{
    T result;
    std::istringstream ss(str);
    ss>>result;
    return result;
}

std::string GetDirPath(const std::string& filepath);
std::string GetFilename(const std::string& filepath);

bool StartsWith(const std::string& str, const std::string& substr);
bool EndsWith(const std::string& str, const std::string& substr);

}  //namespace common
}  //namespace sybie

#endif //ifndef INCLUDE_SYBIE_COMMON_TEXT_HH
