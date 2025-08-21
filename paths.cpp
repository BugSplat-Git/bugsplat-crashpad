#include "paths.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <climits>
#else
#include <unistd.h>
#include <string.h>
#include <climits>
#endif

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// Function to get the executable directory
std::string getExecutableDir()
{
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string pathStr(path);
    size_t lastBackslash = pathStr.find_last_of('\\');
    if (lastBackslash != std::string::npos)
    {
        return pathStr.substr(0, lastBackslash);
    }
    return "";
#elif defined(__APPLE__)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
    {
        std::string pathStr(path);
        size_t lastSlash = pathStr.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            return pathStr.substr(0, lastSlash);
        }
    }
    return "";
#else // Linux
    char pBuf[FILENAME_MAX];
    int len = sizeof(pBuf);
    int bytes = MIN(readlink("/proc/self/exe", pBuf, len), len - 1);
    if (bytes >= 0)
    {
        pBuf[bytes] = '\0';
    }

    char *lastForwardSlash = strrchr(&pBuf[0], '/');
    if (lastForwardSlash == NULL)
        return "";
    *lastForwardSlash = '\0';

    return pBuf;
#endif
}
