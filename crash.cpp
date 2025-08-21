#include <cstddef>
#include <cstdio>
#include <cstring>

extern "C" {
    #ifdef _WIN32
    __declspec(dllexport)
    #else
    __attribute__((visibility("default")))
    #endif
    void crash() {
        // Dereference null pointer to cause a crash
        *(volatile int*)nullptr = 42;
    }

    #ifdef _WIN32
    __declspec(dllexport)
    #else
    __attribute__((visibility("default")))
    #endif
    void crashStackOverflow() {
        // Recursive function that will cause stack overflow
        // Use volatile to prevent compiler optimization
        volatile char buffer[8192];  // Large stack allocation
        buffer[0] = 1;  // Touch the memory to ensure allocation
        
        // Infinite recursion to overflow the stack
        crashStackOverflow();
    }

    #ifdef _WIN32
    __declspec(dllexport)
    #else
    __attribute__((visibility("default")))
    #endif
    void crashAccessViolation() {
        // Try to write to an invalid memory address
        volatile int* invalid_ptr = (volatile int*)0xDEADBEEF;
        *invalid_ptr = 42;
    }

    #ifdef _WIN32
    __declspec(dllexport)
    #else
    __attribute__((visibility("default")))
    #endif
    void crashStackOverrun() {
        // Generate STATUS_STACK_BUFFER_OVERRUN (0xc0000409). This is not catchable by user code.
        // You can use WER to generate a minidump that can be uploaded to BugSplat on the next run.
        printf("Stack buffer overrun starting...\n");

        char buffer[10];

        // This should definitely corrupt the stack canary
        // Writing way beyond the buffer bounds
        memset(buffer, 'A', 2000);  // Write 2000 bytes into a 10-byte buffer

        printf("Stack buffer overrun completed.\n");
        // The error will trigger when the function tries to return
    }
} 