# Windows Error Reporting (WER) Integration

This document explains the WER callback integration that has been added to catch crashes that Crashpad might miss.

## Overview

Windows Error Reporting (WER) can catch certain types of crashes that Crashpad's in-process handlers might miss, particularly:

- **Stack Overflow** (`STATUS_STACK_OVERFLOW`, `STATUS_STACK_BUFFER_OVERRUN`)
- **Heap Corruption** (`STATUS_HEAP_CORRUPTION`)
- **Severe Access Violations** (in certain contexts)

This implementation follows Chrome's approach by creating a separate WER callback DLL that registers with Windows to handle these exception types.

## Architecture

### Components Created

1. **`wer.cpp`** - WER callback DLL implementation
2. **`wer.h`** - Header for WER callback exports
3. **`wer.def`** - Module definition file for DLL exports
4. **WER Registration Functions** - In `main.cpp` for registering/unregistering callbacks

### How It Works

1. **Application Startup**: Main application registers the WER callback DLL
2. **Crash Occurs**: If Crashpad can't handle the crash, WER takes over
3. **WER Callback**: Our DLL receives the exception information
4. **Crashpad Processing**: Uses `crashpad::wer::ExceptionEvent` to process the crash
5. **Report Upload**: Crash gets uploaded to BugSplat through Crashpad's infrastructure

## Exception Types Handled

```cpp
DWORD wanted_exceptions[] = {
    0xC0000409,  // STATUS_STACK_BUFFER_OVERRUN (stack overflow/corruption)
    0xC00000FD,  // STATUS_STACK_OVERFLOW (stack overflow)
    0xC0000374,  // STATUS_HEAP_CORRUPTION (heap corruption)
    0xC0000005,  // STATUS_ACCESS_VIOLATION (in specific contexts)
};
```

## Testing Different Crash Types

In `main.cpp`, function `func2()` contains crash type selection:

```cpp
// 1. NULL POINTER DEREFERENCE (Crashpad handles this well)
// crash_func_t crash_func = loadCrashFunction("crash");

// 2. STACK OVERFLOW (WER catches this better) - CURRENTLY SELECTED
crash_func_t crash_func = loadCrashFunction("crashStackOverflow");

// 3. ACCESS VIOLATION (Both can catch, WER might provide better details)
// crash_func_t crash_func = loadCrashFunction("crashAccessViolation");

// 4. HEAP CORRUPTION (WER often catches these better)
// crash_func_t crash_func = loadCrashFunction("crashHeapCorruption");
```

## Build Process (Windows Only)

The CMakeLists.txt has been updated to:

1. **Build WER DLL**: Creates `wer.dll`
2. **Link WER Libraries**: Links against `crashpad_wer.lib` and `wer.lib`
3. **Copy to Output**: Copies DLL to build directory
4. **Symbol Upload**: Includes WER DLL in symbol uploads

## Runtime Behavior

### Success Case
```
Hello, World!
Crashpad initialized successfully!
Successfully registered WER callback: C:\path\to\wer.dll
WER callbacks registered for enhanced crash detection!
Generating crash...
[Application crashes - WER callback processes it through Crashpad]
```

### Failure Case
```
Hello, World!
Crashpad initialized successfully!
Warning: Failed to register WER callbacks. Some crash types may not be reported.
```

## Platform Behavior

- **Windows**: Full WER integration with callback DLL
- **macOS/Linux**: Standard Crashpad behavior (WER code is conditionally compiled out)

## Benefits

✅ **Enhanced Coverage**: Catches crashes that Crashpad alone might miss  
✅ **Unified Reporting**: All crashes still go through Crashpad → BugSplat  
✅ **Chrome-Tested**: Uses the same approach as Chrome's crash reporting  
✅ **Graceful Fallback**: Application continues if WER registration fails  

## Debugging

To verify WER integration:

1. **Check Registration**: Look for "Successfully registered WER callback" message
2. **Test Stack Overflow**: Use `crashStackOverflow()` function
3. **Event Viewer**: Check Windows Event Viewer for WER events
4. **BugSplat Dashboard**: Verify crash reports appear in BugSplat

## Files Modified/Added

### New Files
- `wer.cpp` - WER callback implementation
- `wer.h` - WER callback header
- `wer.def` - DLL export definitions

### Modified Files
- `CMakeLists.txt` - Added WER DLL build target
- `main.cpp` - Added WER registration functions and calls
- `main.h` - Added WER function declarations
- `crash.cpp` - Added multiple crash type functions

This implementation provides comprehensive crash coverage by combining Crashpad's cross-platform capabilities with Windows-specific WER integration for maximum crash detection coverage.
