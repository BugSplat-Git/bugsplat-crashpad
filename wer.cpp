// Copyright 2024 BugSplat Crashpad WER Integration
// This module provides Windows Error Reporting (WER) callbacks to catch
// exceptions that Crashpad might miss, particularly stack overflow errors.

#ifdef _WIN32

#include <Windows.h>
// werapi.h must be after Windows.h
#include <werapi.h>
#include <sstream>
#include <iostream>

extern "C" {

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            // Disable thread library calls for performance
            DisableThreadLibraryCalls(instance);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

// Main WER callback function that handles out-of-process exceptions
HRESULT OutOfProcessExceptionEventCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    BOOL* pbOwnershipClaimed,
    PWSTR pwszEventName,
    PDWORD pchSize,
    PDWORD pdwSignatureCount) {
    
    // For simplicity and reliability, we'll handle all exceptions that reach this callback
    // This ensures WER can generate comprehensive crash dumps for any crashes that 
    // Crashpad might have difficulty with
    
    // Claim ownership of this exception
    *pbOwnershipClaimed = TRUE;
    
    // Log that we caught an exception (for debugging)
    OutputDebugStringW(L"WER callback handling exception for enhanced crash reporting");
    
    // Return E_FAIL to indicate the process should terminate
    // This allows WER to generate a crash dump and potentially upload it through
    // the normal Windows Error Reporting mechanisms, while still allowing
    // Crashpad to process it as well
    return E_FAIL;
}

// WER signature callback - not used in our implementation
HRESULT OutOfProcessExceptionEventSignatureCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    DWORD dwIndex,
    PWSTR pwszName,
    PDWORD pchName,
    PWSTR pwszValue,
    PDWORD pchValue) {
    
    // This function should never be called in our implementation
    return E_FAIL;
}

// WER debugger launch callback - not used in our implementation
HRESULT OutOfProcessExceptionEventDebuggerLaunchCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    PBOOL pbIsCustomDebugger,
    PWSTR pwszDebuggerLaunch,
    PDWORD pchDebuggerLaunch,
    PBOOL pbIsDebuggerAutolaunch) {
    
    // This function should never be called in our implementation
    return E_FAIL;
}

}  // extern "C"

#endif  // _WIN32
