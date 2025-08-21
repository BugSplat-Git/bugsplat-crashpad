#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <werapi.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <climits>
#else
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#endif

#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include "main.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

using namespace crashpad;
using namespace std;

int main()
{
    // Initialize Crashpad crash reporting (including WER registration on Windows)
    if (!initializeCrashpad(BUGSPLAT_DATABASE, BUGSPLAT_APP_NAME, BUGSPLAT_APP_VERSION))
    {
        std::cerr << "Failed to initialize Crashpad" << std::endl;
        return 1;
    }

    std::cout << "Hello, World!" << std::endl;
    std::cout << "Crashpad initialized successfully!" << std::endl;
    std::cout << "Generating crash..." << std::endl;

    generateExampleCallstackAndCrash();

    return 0;
}

// Function to initialize Crashpad with BugSplat integration
bool initializeCrashpad(std::string dbName, std::string appName, std::string appVersion)
{
    using namespace crashpad;

    std::string exeDir = getExecutableDir();

    // Ensure that crashpad_handler is shipped with your application
#ifdef _WIN32
    base::FilePath handler(base::FilePath::StringType(exeDir.begin(), exeDir.end()) + L"/crashpad_handler.exe");
#else
    base::FilePath handler(exeDir + "/crashpad_handler");
#endif

    // Directory where reports and metrics will be saved
#ifdef _WIN32
    base::FilePath reportsDir(base::FilePath::StringType(exeDir.begin(), exeDir.end()));
    base::FilePath metricsDir(base::FilePath::StringType(exeDir.begin(), exeDir.end()));
#else
    base::FilePath reportsDir(exeDir);
    base::FilePath metricsDir(exeDir);
#endif

    // Configure url with your BugSplat database
    std::string url = "https://" + dbName + ".bugsplat.com/post/bp/crash/crashpad.php";

    // Metadata that will be posted to BugSplat
    std::map<std::string, std::string> annotations;
    annotations["format"] = "minidump";                                    // Required: Crashpad setting to save crash as a minidump
    annotations["database"] = dbName;                                      // Required: BugSplat database
    annotations["product"] = appName;                                      // Required: BugSplat appName
    annotations["version"] = appVersion;                                   // Required: BugSplat appVersion
    annotations["key"] = "Sample key";                                     // Optional: BugSplat key field
    annotations["user"] = "fred@bugsplat.com";                             // Optional: BugSplat user email
    annotations["list_annotations"] = "Sample crash from dynamic library"; // Optional: BugSplat crash description

    // Disable crashpad rate limiting
    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit");

    // File paths of attachments to be uploaded with the minidump file at crash time
    std::vector<base::FilePath> attachments;
#ifdef _WIN32
    // On Windows, attachments are supported
    base::FilePath attachment(base::FilePath::StringType(exeDir.begin(), exeDir.end()) + L"/attachment.txt");
    // Check if file exists before adding as attachment
    if (std::filesystem::exists(attachment.value())) {
        attachments.push_back(attachment);
    }
#elif defined(__linux__)
    // On Linux, attachments are supported
    base::FilePath attachment(exeDir + "/attachment.txt");
    // Check if file exists before adding as attachment
    if (std::filesystem::exists(attachment.value())) {
        attachments.push_back(attachment);
    }
#endif
    // Note: Attachments are not supported on macOS in some Crashpad configurations

    // Initialize Crashpad database
    std::unique_ptr<CrashReportDatabase> database = CrashReportDatabase::Initialize(reportsDir);
    if (database == nullptr)
    {
        return false;
    }

    // Enable automated crash uploads
    Settings *settings = database->GetSettings();
    if (settings == nullptr)
    {
        return false;
    }
    settings->SetUploadsEnabled(true);

    // Start crash handler
    CrashpadClient client;
    bool success = client.StartHandler(
        handler,
        reportsDir,
        metricsDir,
        url,
        annotations,
        arguments,
        true,       // Restartable
        true,       // Asynchronous
        attachments // Add attachment
    );

#ifdef _WIN32
    // Register WER module after starting the handler
    if (success) {
        std::string werDllPath = exeDir + "/wer.dll";
        if (std::filesystem::exists(werDllPath)) {
            std::wstring werDllPathW(werDllPath.begin(), werDllPath.end());
            if (client.RegisterWerModule(werDllPathW)) {
                std::cout << "Successfully registered WER module: " << werDllPath << std::endl;
            } else {
                std::cerr << "Failed to register WER module: " << werDllPath << std::endl;
            }
        }
    }
#endif

    return success;
}



// Struct to manage library handle and provide RAII cleanup
struct LibraryHandle
{
#ifdef _WIN32
    HMODULE handle;
#else
    void *handle;
#endif

    LibraryHandle() : handle(nullptr) {}

    ~LibraryHandle()
    {
        if (handle)
        {
#ifdef _WIN32
            FreeLibrary(handle);
#else
            dlclose(handle);
#endif
        }
    }

    // Non-copyable
    LibraryHandle(const LibraryHandle &) = delete;
    LibraryHandle &operator=(const LibraryHandle &) = delete;
};

// Function to load crash library and return specific crash function pointer
crash_func_t loadCrashFunction(const std::string& functionName)
{
    std::string exeDir = getExecutableDir();

    // Determine library path based on platform
#ifdef _WIN32
    std::string libPath = exeDir + "/crash.dll";
#elif defined(__APPLE__)
    std::string libPath = exeDir + "/libcrash.dylib";
#else // Linux
    std::string libPath = exeDir + "/libcrash.so.2";
#endif

    // Load the library
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(libPath.c_str());
    if (!handle)
    {
        std::cerr << "Failed to load library: " << GetLastError() << std::endl;
        return nullptr;
    }

    // Get the crash function by name
    crash_func_t crash_func = (crash_func_t)GetProcAddress(handle, functionName.c_str());
    if (!crash_func)
    {
        std::cerr << "Failed to get crash function '" << functionName << "': " << GetLastError() << std::endl;
        FreeLibrary(handle);
        return nullptr;
    }
#else
    void *handle = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!handle)
    {
        std::cerr << "Failed to load library: " << dlerror() << std::endl;
        return nullptr;
    }

    // Get the crash function by name
    dlerror(); // Clear any existing error
    crash_func_t crash_func = (crash_func_t)dlsym(handle, functionName.c_str());
    const char *dlsym_error = dlerror();
    if (dlsym_error)
    {
        std::cerr << "Failed to get crash function '" << functionName << "': " << dlsym_error << std::endl;
        dlclose(handle);
        return nullptr;
    }
#endif

    return crash_func;
}

// Convenience function to load the default crash function (null pointer dereference)
crash_func_t loadCrashFunction()
{
    return loadCrashFunction("crash");
}

// Create some dummy frames for a more interesting call stack
void func2()
{
    std::cout << "In func2, loading library and about to crash...\n";

    // ========================================
    // CRASH TYPE SELECTION
    // ========================================
    // Uncomment ONE of the following crash types to test different scenarios:
    
    // 1. NULL POINTER DEREFERENCE (Crashpad handles this well)
    // crash_func_t crash_func = loadCrashFunction("crash");
    
    // 2. STACK OVERFLOW (WER catches this better on Windows) - CURRENTLY SELECTED
    crash_func_t crash_func = loadCrashFunction("crashStackOverflow");
    
    // 3. ACCESS VIOLATION (Both can catch, but WER might provide better details)
    // crash_func_t crash_func = loadCrashFunction("crashAccessViolation");
    
    // 4. HEAP CORRUPTION (WER often catches these better)
    // crash_func_t crash_func = loadCrashFunction("crashHeapCorruption");
    
    if (!crash_func)
    {
        std::cerr << "Failed to load crash function from library" << std::endl;
        return;
    }

    std::cout << "About to call crash function...\n";
    
    // Call the selected crash function
    crash_func();

    // We should never reach here
}

void func1()
{
    std::cout << "In func1, calling func2...\n";
    func2();
}

void func0()
{
    std::cout << "In func0, calling func1...\n";
    func1();
}

void generateExampleCallstackAndCrash()
{
    std::cout << "Starting call chain...\n";
    func0();
}

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