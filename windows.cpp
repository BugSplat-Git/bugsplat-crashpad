#ifdef _WIN32

#include "windows.h"
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <winreg.h>

BOOL DoesRegistryValueExist(HKEY hRootKey, LPCWSTR keyPath, LPCWSTR valueName)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyExW(hRootKey, keyPath, 0, KEY_READ, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return FALSE;  // Key doesn't exist
    }
    
    // Check if the specific value exists
    result = RegQueryValueExW(hKey, valueName, NULL, NULL, NULL, NULL);
    RegCloseKey(hKey);
    
    return (result == ERROR_SUCCESS);
}

// Check for RuntimeExceptionHelperModules key
BOOL CheckRuntimeExceptionHelper(const std::string& dllPath)
{
    const LPCWSTR keyPath = L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\RuntimeExceptionHelperModules";
        
    // Create the value name we're looking for
    std::wstring wideDllPath(dllPath.begin(), dllPath.end());
    WCHAR valueName[2048];
    swprintf_s(valueName, 2048, wideDllPath.c_str());
    
    // Check if our specific value exists
    BOOL exists = DoesRegistryValueExist(HKEY_LOCAL_MACHINE, keyPath, valueName);
    
    return exists;
}


void setupWerIntegration(crashpad::CrashpadClient& client, const std::string& exeDir) {
    std::string werDllPath = exeDir + "\\crashpad_wer.dll";    
    if (!std::filesystem::exists(werDllPath)) {
        std::cout << "Crashpad WER DLL not found at: " << werDllPath << " - continuing without WER integration" << std::endl;
        return;
    }
      
    // Check registry key for WER integration
    bool registryExists = CheckRuntimeExceptionHelper(werDllPath);
    if (!registryExists) {
        std::cout << "Crashpad WER registry key: " << werDllPath << " not found" << std::endl;
        std::cout << "Create this registry value in HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\RuntimeExceptionHelperModules" << std::endl;
        std::cout << "Continuing without WER integration" << std::endl;
        return; 
    }
    
    // Register WER module with Crashpad
    std::wstring werDllPathW(werDllPath.begin(), werDllPath.end());
    bool moduleRegistered = client.RegisterWerModule(werDllPathW);
    
    // Report results
    if (!moduleRegistered) {
        std::cerr << "Warning: Failed to register WER module with Crashpad - continuing without WER integration" << std::endl;
        return;
    }
    
    std::cout << "Successfully set up WER integration: " << werDllPath << std::endl;
}

#endif // _WIN32
