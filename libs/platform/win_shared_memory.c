#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/platform/shared_memory.h"
#include "../../include/log/logger.h"

struct SharedMemoryHandle
{
    HANDLE handle;
};

SharedMemoryHandle *create_shared_memory(const char *name, size_t size)
{
    SharedMemoryHandle *handle = (SharedMemoryHandle *)malloc(sizeof(SharedMemoryHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for shared memory handle");
        return NULL;
    }

    handle->handle = CreateFileMapping(
        INVALID_HANDLE_VALUE, // Use paging file
        NULL,                 // Default security
        PAGE_READWRITE,       // Read/write access
        0,                    // Max size (high-order DWORD)
        (DWORD)size,          // Max size (low-order DWORD)
        name                  // Name
    );

    if (handle->handle == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not create file mapping object (%lu).", GetLastError());
        error(errorMsg);
        free(handle);
        return NULL;
    }

    return handle;
}

SharedMemoryHandle *open_shared_memory(const char *name)
{
    SharedMemoryHandle *handle = (SharedMemoryHandle *)malloc(sizeof(SharedMemoryHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for shared memory handle");
        return NULL;
    }

    handle->handle = OpenFileMapping(
        FILE_MAP_ALL_ACCESS, // Read/write access
        FALSE,               // Do not inherit the name
        name                 // Name of mapping object
    );

    if (handle->handle == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not open file mapping object (%lu).", GetLastError());
        error(errorMsg);
        free(handle);
        return NULL;
    }

    return handle;
}

void *map_shared_memory(SharedMemoryHandle *handle, size_t size)
{
    if (handle == NULL || handle->handle == NULL)
    {
        return NULL;
    }

    void *data = MapViewOfFile(
        handle->handle,
        FILE_MAP_ALL_ACCESS, // Read/write access
        0,                   // File offset (high-order DWORD)
        0,                   // File offset (low-order DWORD)
        size                 // Number of bytes to map
    );

    if (data == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not map view of file (%lu).", GetLastError());
        error(errorMsg);
    }

    return data;
}

bool unmap_shared_memory(void *data)
{
    if (data == NULL)
    {
        return 0;
    }

    return UnmapViewOfFile(data) != 0;
}

bool close_shared_memory(SharedMemoryHandle *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    bool result = CloseHandle(handle->handle) != 0;
    free(handle);
    return result;
}

#endif // _WIN32