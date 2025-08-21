// Copyright 2024 BugSplat Crashpad WER Integration
// This module provides Windows Error Reporting (WER) callbacks to catch
// exceptions that Crashpad might miss, particularly stack overflow errors.

#ifdef _WIN32

#include <Windows.h>
// werapi.h must be after Windows.h
#include <werapi.h>

// Note: crashpad::wer::ExceptionEvent may not be available in public Crashpad builds
// We'll implement a standalone WER callback that works with Crashpad's RegisterWerModule()

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
    
    // Log that our callback was invoked
    OutputDebugStringW(L"WER callback handling exception for enhanced crash reporting");
    
    // Exceptions that are not collected by crashpad's in-process handlers
    // Based on Chrome's implementation - focus on stack overflow crashes
    DWORD wanted_exceptions[] = {
        0xC0000409,  // STATUS_STACK_BUFFER_OVERRUN (stack overflow/corruption)
        0xC00000FD,  // STATUS_STACK_OVERFLOW (stack overflow)  
        0xC0000374,  // STATUS_HEAP_CORRUPTION (heap corruption)
    };
    
    // Use Crashpad's WER integration to handle these exceptions
    bool result = crashpad::wer::ExceptionEvent(
        wanted_exceptions, 
        sizeof(wanted_exceptions) / sizeof(DWORD), 
        pContext,
        pExceptionInformation);

    if (result) {
        // Crashpad successfully handled the exception
        *pbOwnershipClaimed = TRUE;
        OutputDebugStringW(L"WER callback handling exception for enhanced crash reporting - Crashpad handled exception - ownership claimed");
        
        // Technically we failed as we terminated the process
        return E_FAIL;
    }
    
    // Could not dump for whatever reason, so let other helpers/WER have a chance
    OutputDebugStringW(L"WER CALLBACK: Crashpad could not handle - letting other handlers try");
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
