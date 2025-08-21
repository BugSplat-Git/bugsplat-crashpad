// Copyright 2024 BugSplat Crashpad WER Integration
// This module provides Windows Error Reporting (WER) callbacks to catch
// exceptions that Crashpad might miss, particularly stack overflow errors.

#ifdef _WIN32

#include <Windows.h>
// werapi.h must be after Windows.h
#include <werapi.h>

#include "third_party/crashpad/handler/win/wer/crashpad_wer.h"

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
    
    // List of exception types that WER should handle instead of Crashpad
    // These are exceptions that Crashpad typically doesn't catch well
    DWORD wanted_exceptions[] = {
        0xC0000409,  // STATUS_STACK_BUFFER_OVERRUN (stack overflow/corruption)
        0xC00000FD,  // STATUS_STACK_OVERFLOW (stack overflow)
        0xC0000374,  // STATUS_HEAP_CORRUPTION (heap corruption)
        0xC0000005,  // STATUS_ACCESS_VIOLATION (when in specific contexts)
    };
    
    // Use Crashpad's WER integration to handle these exceptions
    bool result = crashpad::wer::ExceptionEvent(
        wanted_exceptions, 
        sizeof(wanted_exceptions) / sizeof(DWORD), 
        pContext,
        pExceptionInformation);
    
    if (result) {
        // We successfully handled the exception
        *pbOwnershipClaimed = TRUE;
        
        // Return E_FAIL to indicate we've terminated the process
        // This tells WER that we've handled it but the process should still terminate
        return E_FAIL;
    }
    
    // Could not handle the exception, let other WER handlers or default WER try
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
