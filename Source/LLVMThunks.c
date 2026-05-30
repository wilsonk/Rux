/*
    Rux Compiler
    Copyright © 2026 Rux Contributors
    Licensed under the MIT License

    Platform-specific I/O thunks for LLVM backend
    These provide Windows API compatibility on Linux/macOS
*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

// GetStdHandle thunk
// -10: STD_INPUT_HANDLE  -> returns 0 (stdin)
// -11: STD_OUTPUT_HANDLE -> returns 1 (stdout)
// -12: STD_ERROR_HANDLE  -> returns 2 (stderr)
long GetStdHandle(long handle) {
    if (handle == -10) return 0;  // STD_INPUT_HANDLE
    if (handle == -11) return 1;  // STD_OUTPUT_HANDLE
    if (handle == -12) return 2;  // STD_ERROR_HANDLE
    return -1;
}

// WriteFile thunk
// Simplified version that writes to file descriptor
long WriteFile(long handle, void* buffer, long bytesToWrite, long* bytesWritten, long overlapped) {
    ssize_t result = write((int)handle, buffer, (size_t)bytesToWrite);
    if (bytesWritten) *bytesWritten = (long)result;
    return result > 0 ? 1 : 0;
}

// ReadFile thunk
// Simplified version that reads from file descriptor
long ReadFile(long handle, void* buffer, long bytesToRead, long* bytesRead, long overlapped) {
    ssize_t result = read((int)handle, buffer, (size_t)bytesToRead);
    if (bytesRead) *bytesRead = (long)result;
    return result > 0 ? 1 : 0;
}

// Memory allocation thunks
// These provide Windows HeapAlloc/HeapFree compatibility
// Note: These are simple wrappers around malloc/free
// For production use, you might want to use a custom allocator

void* HeapAlloc(long heapHandle, long flags, long size) {
    (void)heapHandle; // Unused - we use the system heap
    (void)flags;     // Unused - no special allocation flags
    return malloc((size_t)size);
}

long HeapFree(long heapHandle, long flags, void* ptr) {
    (void)heapHandle; // Unused - we use the system heap
    (void)flags;     // Unused
    if (ptr) {
        free(ptr);
        return 1; // Success
    }
    return 0; // Failure
}
