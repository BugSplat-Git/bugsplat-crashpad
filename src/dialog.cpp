// Sample crash dialog application for Windows
// This demonstrates how to create a dialog that communicates with crashpad_handler

#include <windows.h>
#include <string>
#include <iostream>

// Dialog communication structures (must match crashpad_handler)
struct DialogRequest {
  char report_id[37];  // UUID string + null terminator
  char process_name[MAX_PATH];
};

struct DialogResponse {
  bool should_upload;
  char user_email[256];
  char user_description[1024];
};

const wchar_t kDialogPipeName[] = L"\\\\.\\pipe\\CrashpadDialogPipe";

// Global variables for dialog
DialogResponse g_response = {false, "", ""};

// Main function
int main(int argc, char* argv[]) {
    // Parse command line for report ID
    std::string report_id = (argc > 1) ? argv[1] : "unknown";

    // Remove any leading/trailing whitespace
    size_t start = report_id.find_first_not_of(" \t\r\n");
    size_t end = report_id.find_last_not_of(" \t\r\n");
    if (start != std::string::npos && end != std::string::npos) {
        report_id = report_id.substr(start, end - start + 1);
    }

    // Create a simple message box dialog for user input
    std::string message = "An unexpected error occurred in the application.\n\n";
    message += "Report ID: " + report_id + "\n\n";
    message += "Would you like to send this crash report?";

    int result = MessageBoxA(nullptr,
                           message.c_str(),
                           "Application Crash Report",
                           MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL);

    // Set response based on user choice
    if (result == IDYES) {
        g_response.should_upload = true;
        strcpy_s(g_response.user_email, "user@dialog.example");
        strcpy_s(g_response.user_description, "User chose to send crash report");
    } else {
        g_response.should_upload = false;
        strcpy_s(g_response.user_email, "cancelled@dialog.invalid");
        strcpy_s(g_response.user_description, "User chose not to send crash report");
    }

    // Connect to named pipe to communicate with crashpad_handler
    HANDLE pipe = CreateFileW(kDialogPipeName,
                            GENERIC_READ | GENERIC_WRITE,  // Need both for bidirectional communication
                            0,
                            nullptr,
                            OPEN_EXISTING,
                            0,
                            nullptr);

    if (pipe != INVALID_HANDLE_VALUE) {
        // First, read the request from crashpad_handler
        DialogRequest request;
        DWORD bytes_read;
        BOOL read_result = ReadFile(pipe, &request, sizeof(request), &bytes_read, nullptr);

        if (read_result && bytes_read == sizeof(request)) {
            // Now send the response back
            DWORD bytes_written;
            BOOL write_result = WriteFile(pipe, &g_response, sizeof(g_response), &bytes_written, nullptr);

            if (write_result && bytes_written == sizeof(g_response)) {
                // Response sent successfully
            }
        }

        CloseHandle(pipe);
    }

    return (result == IDYES) ? 0 : 1;
}

// Dialog resource (IDD_CRASH_DIALOG)
// This would typically be in a .rc file, but included inline for simplicity
/*
IDD_CRASH_DIALOG DIALOGEX 0, 0, 400, 250
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Application Crash Report"
FONT 8, "MS Sans Serif"
BEGIN
    LTEXT           "An unexpected error occurred in the application.", IDC_STATIC, 10, 10, 380, 20
    LTEXT           "Report ID:", IDC_STATIC, 10, 35, 50, 15
    LTEXT           "ReportIdValue", IDC_REPORT_ID, 65, 35, 200, 15

    LTEXT           "Your Email:", IDC_STATIC, 10, 60, 50, 15
    EDITTEXT        IDC_EMAIL_EDIT, 65, 58, 200, 14, ES_AUTOHSCROLL

    LTEXT           "Description (optional):", IDC_STATIC, 10, 85, 80, 15
    EDITTEXT        IDC_DESCRIPTION_EDIT, 10, 100, 380, 80, ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL

    DEFPUSHBUTTON   "Send Report", IDC_SEND_BUTTON, 310, 190, 80, 14
    PUSHBUTTON      "Don't Send", IDC_CANCEL_BUTTON, 220, 190, 80, 14

    LTEXT           "Your email helps us follow up on this issue. Reports are sent anonymously unless you provide contact information.", IDC_STATIC, 10, 210, 380, 30
END
*/

// Control IDs
#define IDC_EMAIL_EDIT      1001
#define IDC_DESCRIPTION_EDIT 1002
#define IDC_SEND_BUTTON     IDOK
#define IDC_CANCEL_BUTTON   IDCANCEL
#define IDC_REPORT_ID       1003
#define IDC_STATIC          -1

