#ifndef WINDOWS_H
#define WINDOWS_H

#ifdef _WIN32

#include <string>
#include "client/crashpad_client.h"

// Windows-specific WER (Windows Error Reporting) integration functions

/**
 * Creates a Windows registry key for WER integration.
 * Registers the crashpad_wer.dll path in HKEY_CURRENT_USER for Windows Error Reporting.
 * 
 * @param dllPath Full absolute path to crashpad_wer.dll
 * @return true if registry key was created successfully, false otherwise
 */
bool createWerRegistryKey(const std::string& dllPath);

/**
 * Sets up complete WER integration for Crashpad.
 * This function orchestrates both registry setup and Crashpad WER module registration.
 * 
 * @param client Reference to the initialized CrashpadClient
 * @param exeDir Directory where the executable (and crashpad_wer.dll) are located
 */
void setupWerIntegration(crashpad::CrashpadClient& client, const std::string& exeDir);

#endif // _WIN32

#endif // WINDOWS_H
