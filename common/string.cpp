#include "string.hpp"
#include "windows.h"
#include <algorithm>
#include <codecvt>
#include <locale>

namespace string
{
    std::wstring _StringToWstring(const std::string &oString)
    {
        int iBufferSize = MultiByteToWideChar(CP_ACP, 0, oString.c_str(), -1, (wchar_t *)NULL, 0);
        wchar_t *cpUCS2 = new wchar_t[iBufferSize];
        MultiByteToWideChar(CP_ACP, 0, oString.c_str(), -1, cpUCS2, iBufferSize);
        std::wstring oRet(cpUCS2, cpUCS2 + iBufferSize - 1);
        delete[] cpUCS2;
        return {oRet};
    }

    std::string _WstringToString(std::wstring oWString)
    {
        int iBufferSize = WideCharToMultiByte(CP_OEMCP, 0, oWString.c_str(), -1, (char *)NULL, 0, NULL, NULL);
        CHAR *cpMultiByte = new CHAR[iBufferSize];
        WideCharToMultiByte(CP_OEMCP, 0, oWString.c_str(), -1, cpMultiByte, iBufferSize, NULL, NULL);
        std::string oRet(cpMultiByte, cpMultiByte + iBufferSize - 1);
        delete[] cpMultiByte;
        return {oRet};
    }

    std::string toMultiByte(const std::string &s) { return s; }
    std::string toMultiByte(const std::wstring &s) { return _WstringToString(s); }
    std::wstring toWideChar(const std::string &s) { return _StringToWstring(s); }
    std::wstring toWideChar(const std::wstring &s) { return s; }
#ifdef UNICODE
    String toString(const std::string &s)
    {
        return toWideChar(s);
    }
    String toString(const std::wstring &s)
    {
        return toWideChar(s);
    }
#else
    String toString(const std::string &s)
    {
        return toMultiByte(s);
    }
    String toString(const std::wstring &s)
    {
        return toMultiByte(s);
    }
#endif

    String lower(const String &s)
    {
        String res{s};
        std::transform(res.begin(), res.end(), res.begin(),
                       [](Char c)
                       { return std::tolower(c); });
        return res;
    }

    std::wstring utf8_to_wstring(std::string const &src)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(src);
    }
    std::string wstring_to_utf8(std::wstring const &src)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(src);
    }
}
