#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include "windows.h"
#elif defined(__APPLE__)
#include <dlfcn.h>
#else
#include <dlfcn.h>
#endif

#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include "main.h"
#include "paths.h"

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

void func0()
{
    std::cout << "In func0, calling func1..." << std::endl;
    func1();
}

void func1()
{
    std::cout << "In func1, calling func2..." << std::endl;
    func2();
}

// Create some dummy frames for a more interesting call stack
void func2()
{
    std::cout << "In func2, loading library and about to crash..." << std::endl;

    // ========================================
    // CRASH TYPE SELECTION
    // ========================================
    // Uncomment ONE of the following crash types to test different scenarios:
    
    // 1. NULL POINTER DEREFERENCE
    crash_func_t crash_func = loadCrashFunction("crash");

    // 2. ACCESS VIOLATION
    // crash_func_t crash_func = loadCrashFunction("crashAccessViolation");
    
    // 3. STACK OVERFLOW
    // crash_func_t crash_func = loadCrashFunction("crashStackOverflow");
    
    // 4. STACK BUFFER OVERWRITE (WER callback required for Windows see https://github.com/BugSplat-Git/bugsplat-crashpad/wiki/WER)
    // crash_func_t crash_func = loadCrashFunction("crashStackOverwrite");
    
    if (!crash_func)
    {
        std::cerr << "Failed to load crash function from library" << std::endl;
        return;
    }

    std::cout << "About to call crash function..." << std::endl;
    crash_func();
}

void generateExampleCallstackAndCrash()
{
    std::cout << "Starting call chain..." << std::endl;
    func0();
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
    // Set up Windows Error Reporting (WER) integration if Crashpad initialization was successful.
    //
    // REGISTRY REQUIREMENT:
    // WER integration requires a specific registry configuration in:
    //   HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\RuntimeExceptionHelperModules
    //
    // IMPORTANT: Creating registry values in HKEY_LOCAL_MACHINE requires elevated privileges.
    //            This configuration should typically be created by your installer or setup process.
    // 
    // You must create a DWORD registry VALUE with:
    //   Name: [full_path_to_crashpad_wer.dll]
    //   Type: REG_DWORD
    //   Data: 0 (value contents are ignored by WER)
    //
    // Example registry value:
    //   Name: "C:\MyApp\bugsplat-crashpad\build\Debug\crashpad_wer.dll"
    //   Type: REG_DWORD
    //   Data: 0x00000000 (0)
    //
    // This configuration allows WER to delegate crash handling to your Crashpad handler.
    if (success) {
        setupWerIntegration(client, exeDir);
    }
#endif

    return success;
}

// Function to load crash library and return specific crash function pointer
crash_func_t loadCrashFunction(const std::string& functionName)
{
    std::string exeDir = getExecutableDir();

    // Determine library path based on platform
#ifdef _WIN32
    std::string libPath = exeDir + "\\crash.dll";
#elif defined(__APPLE__)
    std::string libPath = exeDir + "/libcrash.dylib";
#else // Linux
    std::string libPath = exeDir + "/libcrash.so.2";
#endif

    std::cout << "Loading crash function '" << functionName << "' from library: " << libPath << std::endl;

    // Load the library
#ifdef _WIN32
    // Load the library using LoadLibrary
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
