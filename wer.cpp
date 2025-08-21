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
    
    // List of exception types that WER should handle for better crash reporting
    // These are exceptions that Crashpad sometimes has difficulty with
    DWORD wanted_exceptions[] = {
        0xC0000409,  // STATUS_STACK_BUFFER_OVERRUN (stack overflow/corruption)
        0xC00000FD,  // STATUS_STACK_OVERFLOW (stack overflow)
        0xC0000374,  // STATUS_HEAP_CORRUPTION (heap corruption)
        0xC0000005,  // STATUS_ACCESS_VIOLATION (when in specific contexts)
    };
    
    // Check if this is an exception type we want to handle
    DWORD exceptionCode = pExceptionInformation->ExceptionRecord.ExceptionCode;
    bool shouldHandle = false;
    
    for (int i = 0; i < sizeof(wanted_exceptions) / sizeof(DWORD); i++) {
        if (exceptionCode == wanted_exceptions[i]) {
            shouldHandle = true;
            break;
        }
    }
    
    if (shouldHandle) {
        // We want to handle this exception type
        *pbOwnershipClaimed = TRUE;
        
        // Log that we caught this exception (optional debugging)
        std::wostringstream logStream;
        logStream << L"WER callback caught exception: 0x" << std::hex << exceptionCode;
        OutputDebugStringW(logStream.str().c_str());
        
        // Return E_FAIL to indicate the process should terminate
        // This allows WER to generate a crash dump through normal Windows mechanisms
        return E_FAIL;
    }
    
    // This is not an exception type we specifically handle
    *pbOwnershipClaimed = FALSE;
    return S_OK;
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
