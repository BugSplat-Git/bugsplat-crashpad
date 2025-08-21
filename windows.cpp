#ifdef _WIN32

#include "windows.h"
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <winreg.h>

bool createWerRegistryKey(const std::string& dllPath) {
    const char* registryPath = "SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\RuntimeExceptionHelperModules";
    HKEY hKey;
    
    // Open/create the registry key
    LONG result = RegCreateKeyExA(
        HKEY_CURRENT_USER,
        registryPath,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL
    );
    
    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to open/create WER registry key. Error: " << result << std::endl;
        return false;
    }
    
    // Set the DLL path as a DWORD value with data 0x0
    DWORD value = 0x0;
    result = RegSetValueExA(
        hKey,
        dllPath.c_str(),  // Value name is the full DLL path
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(DWORD)
    );
    
    RegCloseKey(hKey);
    
    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to set WER registry value. Error: " << result << std::endl;
        return false;
    }
    
    std::cout << "Successfully created WER registry key for: " << dllPath << std::endl;
    return true;
}

void setupWerIntegration(crashpad::CrashpadClient& client, const std::string& exeDir) {
    std::string werDllPath = exeDir + "\\crashpad_wer.dll";
    std::cout << "Looking for Crashpad WER DLL at: " << werDllPath << std::endl;
    
    if (!std::filesystem::exists(werDllPath)) {
        std::cout << "Crashpad WER DLL not found - continuing without WER integration" << std::endl;
        return;
    }
    
    std::cout << "Crashpad WER DLL found, setting up WER integration..." << std::endl;
    
    // Create registry key for WER integration
    bool registryCreated = createWerRegistryKey(werDllPath);
    
    // Register WER module with Crashpad
    std::wstring werDllPathW(werDllPath.begin(), werDllPath.end());
    bool moduleRegistered = client.RegisterWerModule(werDllPathW);
    
    // Report results
    if (registryCreated && moduleRegistered) {
        std::cout << "Successfully set up WER integration: " << werDllPath << std::endl;
        return;
    }
    
    // Handle partial or complete failure
    if (!registryCreated) {
        std::cerr << "Warning: Failed to create WER registry key" << std::endl;
    }
    if (!moduleRegistered) {
        std::cerr << "Warning: Failed to register WER module with Crashpad" << std::endl;
    }
    std::cerr << "WER integration may not work properly" << std::endl;
}

#endif // _WIN32
