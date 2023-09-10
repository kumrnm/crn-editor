#pragma once

#include <string>
#include <sstream>

#ifdef UNICODE

using String = std::wstring;
using Char = wchar_t;
using StringStream = std::wstringstream;

#else

using String = std::string;
using Char = char;
using StringStream = std::stringstream;

#endif

namespace string
{
    String toString(const std::string &);
    String toString(const std::wstring &);
    std::string toMultiByte(const std::string &);
    std::string toMultiByte(const std::wstring &);
    std::wstring toWideChar(const std::string &);
    std::wstring toWideChar(const std::wstring &);

    String lower(const String &);

    std::wstring utf8_to_wstring(std::string const &);
    std::string wstring_to_utf8(std::wstring const &);
}
