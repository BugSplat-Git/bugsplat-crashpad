// Crash dialog application for Windows with email and comment input
// This demonstrates how to create a dialog that communicates with crashpad_handler

#include <windows.h>
#include <commctrl.h>
#include <string>

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

// Control IDs
#define IDC_REPORT_ID_LABEL     1001
#define IDC_EMAIL_LABEL         1002
#define IDC_EMAIL_EDIT          1003
#define IDC_DESCRIPTION_LABEL   1004
#define IDC_DESCRIPTION_EDIT    1005
#define IDC_SEND_BUTTON         1006
#define IDC_CANCEL_BUTTON       1007
#define IDC_HEADER_LABEL        1008

// Global variables
DialogResponse g_response = {false, "", ""};
std::string g_report_id;
HWND g_hEmailEdit = nullptr;
HWND g_hDescriptionEdit = nullptr;

// Dialog procedure
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            // Center the dialog on screen
            RECT rc;
            GetWindowRect(hwndDlg, &rc);
            int xPos = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
            int yPos = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
            SetWindowPos(hwndDlg, nullptr, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            // Set dialog title
            SetWindowTextA(hwndDlg, "Application Crash Report");

            // Create header label
            HWND hHeader = CreateWindowExA(0, "STATIC", 
                "An unexpected error occurred in the application.",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 20, 460, 20,
                hwndDlg, (HMENU)IDC_HEADER_LABEL, GetModuleHandle(nullptr), nullptr);

            // Create report ID label
            std::string reportIdText = "Report ID: " + g_report_id;
            CreateWindowExA(0, "STATIC", reportIdText.c_str(),
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 50, 460, 20,
                hwndDlg, (HMENU)IDC_REPORT_ID_LABEL, GetModuleHandle(nullptr), nullptr);

            // Create email label
            CreateWindowExA(0, "STATIC", "Email (optional):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 85, 460, 20,
                hwndDlg, (HMENU)IDC_EMAIL_LABEL, GetModuleHandle(nullptr), nullptr);

            // Create email edit control
            g_hEmailEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                20, 105, 460, 25,
                hwndDlg, (HMENU)IDC_EMAIL_EDIT, GetModuleHandle(nullptr), nullptr);

            // Create description label
            CreateWindowExA(0, "STATIC", "Additional comments (optional):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 145, 460, 20,
                hwndDlg, (HMENU)IDC_DESCRIPTION_LABEL, GetModuleHandle(nullptr), nullptr);

            // Create description multiline edit control
            g_hDescriptionEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | 
                ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL,
                20, 165, 460, 100,
                hwndDlg, (HMENU)IDC_DESCRIPTION_EDIT, GetModuleHandle(nullptr), nullptr);

            // Create Send button
            CreateWindowExA(0, "BUTTON", "Send Report",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                380, 280, 100, 30,
                hwndDlg, (HMENU)IDC_SEND_BUTTON, GetModuleHandle(nullptr), nullptr);

            // Create Cancel button
            CreateWindowExA(0, "BUTTON", "Don't Send",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                270, 280, 100, 30,
                hwndDlg, (HMENU)IDC_CANCEL_BUTTON, GetModuleHandle(nullptr), nullptr);

            // Set focus to the Send button
            SetFocus(GetDlgItem(hwndDlg, IDC_SEND_BUTTON));
            return FALSE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_SEND_BUTTON: {
                    // Get email text
                    GetWindowTextA(g_hEmailEdit, g_response.user_email, sizeof(g_response.user_email));
                    
                    // Get description text
                    GetWindowTextA(g_hDescriptionEdit, g_response.user_description, sizeof(g_response.user_description));
                    
                    // Set upload flag
                    g_response.should_upload = true;
                    
                    EndDialog(hwndDlg, IDOK);
                    return TRUE;
                }

                case IDC_CANCEL_BUTTON: {
                    // User chose not to send
                    g_response.should_upload = false;
                    strcpy_s(g_response.user_email, "");
                    strcpy_s(g_response.user_description, "User declined to send crash report");
                    
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
                }
            }
            break;
        }

        case WM_CLOSE:
            // Treat close as cancel
            g_response.should_upload = false;
            strcpy_s(g_response.user_email, "");
            strcpy_s(g_response.user_description, "User closed dialog without sending");
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
    }

    return FALSE;
}

// Create in-memory dialog template
LPCDLGTEMPLATE CreateDialogTemplate() {
    // Allocate memory for dialog template
    HGLOBAL hGlobal = GlobalAlloc(GMEM_ZEROINIT, 1024);
    LPDLGTEMPLATE pTemplate = (LPDLGTEMPLATE)GlobalLock(hGlobal);

    // Define dialog
    pTemplate->style = DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    pTemplate->cdit = 0;  // No items in template, we'll create them manually
    pTemplate->x = 0;
    pTemplate->y = 0;
    pTemplate->cx = 260;  // Dialog units
    pTemplate->cy = 180;

    GlobalUnlock(hGlobal);
    return pTemplate;
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Parse command line for report ID
    g_report_id = (lpCmdLine && strlen(lpCmdLine) > 0) ? lpCmdLine : "unknown";

    // Remove any leading/trailing whitespace and quotes
    size_t start = g_report_id.find_first_not_of(" \t\r\n\"");
    size_t end = g_report_id.find_last_not_of(" \t\r\n\"");
    if (start != std::string::npos && end != std::string::npos) {
        g_report_id = g_report_id.substr(start, end - start + 1);
    }

    // Create and show the dialog
    LPCDLGTEMPLATE pTemplate = CreateDialogTemplate();
    INT_PTR result = DialogBoxIndirectParam(
        hInstance,
        pTemplate,
        nullptr,
        DialogProc,
        0);

    GlobalFree((HGLOBAL)pTemplate);

    // Connect to named pipe to communicate with crashpad_handler
    HANDLE pipe = CreateFileW(kDialogPipeName,
                            GENERIC_READ | GENERIC_WRITE,
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
            WriteFile(pipe, &g_response, sizeof(g_response), &bytes_written, nullptr);
        }

        CloseHandle(pipe);
    }

    return g_response.should_upload ? 0 : 1;
}

