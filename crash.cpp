#include <cstddef>

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
    void crashHeapCorruption() {
        // Allocate memory and then corrupt it
        int* ptr = new int[10];
        delete[] ptr;
        // Use after free - this can cause heap corruption
        *ptr = 42;
    }
} 