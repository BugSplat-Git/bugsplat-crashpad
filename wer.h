// Copyright 2024 BugSplat Crashpad WER Integration
// Header file for Windows Error Reporting (WER) callback functionality

#ifndef WER_H
#define WER_H

#ifdef _WIN32

#include <Windows.h>
#include <werapi.h>

extern "C" {

// WER callback functions that need to be exported from the DLL
__declspec(dllexport) HRESULT OutOfProcessExceptionEventCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    BOOL* pbOwnershipClaimed,
    PWSTR pwszEventName,
    PDWORD pchSize,
    PDWORD pdwSignatureCount);

__declspec(dllexport) HRESULT OutOfProcessExceptionEventSignatureCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    DWORD dwIndex,
    PWSTR pwszName,
    PDWORD pchName,
    PWSTR pwszValue,
    PDWORD pchValue);

__declspec(dllexport) HRESULT OutOfProcessExceptionEventDebuggerLaunchCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    PBOOL pbIsCustomDebugger,
    PWSTR pwszDebuggerLaunch,
    PDWORD pchDebuggerLaunch,
    PBOOL pbIsDebuggerAutolaunch);

}  // extern "C"

#endif  // _WIN32

#endif  // WER_H
