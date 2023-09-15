#include "file.hpp"
#include <windows.h>

std::vector<String> getFileList(const String &fileName)
{
    std::vector<String> result;

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(fileName.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return {};
    }
    else
    {
        do
        {
            result.emplace_back(ffd.cFileName);
        } while (FindNextFile(hFind, &ffd));

        FindClose(hFind);
        return result;
    }
}

String fixFileName(const String &str)
{
    String res;
    for (const auto &c : str)
    {
        switch (c)
        {
        case TEXT('\\'):
        case TEXT('/'):
        case TEXT(':'):
        case TEXT('*'):
        case TEXT('?'):
        case TEXT('"'):
        case TEXT('<'):
        case TEXT('>'):
        case TEXT('|'):
        case TEXT('_'):
            continue;
        default:
            res.push_back(c);
        }
    }
    return res;
}
