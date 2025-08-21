#ifndef MAIN_H
#define MAIN_H

#include <string>

// BugSplat Configuration
// Replace BUGSPLAT_DATABASE with your database name from your BugSplat dashboard
// #define BUGSPLAT_DATABASE "fred"
#ifndef BUGSPLAT_DATABASE
#error "BUGSPLAT_DATABASE must be defined. Please set it to your database name from your BugSplat dashboard."
#endif

// Application name and version
#define BUGSPLAT_APP_NAME "MyCMakeCrasher"
#define BUGSPLAT_APP_VERSION "1.0"

// Function type for the crash function
typedef void (*crash_func_t)(void);

// Function declarations
std::string getExecutableDir();
bool initializeCrashpad(std::string dbName, std::string appName, std::string appVersion);
crash_func_t loadCrashFunction();
crash_func_t loadCrashFunction(const std::string& functionName);
void generateExampleCallstackAndCrash();
void func0();
void func1();
void func2();

#endif // MAIN_H 