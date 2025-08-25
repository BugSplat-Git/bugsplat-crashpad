#ifndef WINDOWS_H
#define WINDOWS_H

#ifdef _WIN32

#include <string>
#include "client/crashpad_client.h"

// Windows-specific WER (Windows Error Reporting) integration functions


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
