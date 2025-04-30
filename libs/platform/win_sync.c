#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "../../include/platform/sync.h"
#include "../../include/log/logger.h"

struct MutexHandle
{
    HANDLE handle;
};

struct SemaphoreHandle
{
    HANDLE handle;
};

// Mutex operations
MutexHandle *create_mutex(const char *name)
{
    MutexHandle *handle = (MutexHandle *)malloc(sizeof(MutexHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for mutex handle");
        return NULL;
    }

    handle->handle = CreateMutex(
        NULL,  // Default security attributes
        FALSE, // Initially not owned
        (name) // Named mutex
    );

    if (handle->handle == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not create mutex (%lu).", GetLastError());
        error(errorMsg);
        free(handle);
        return NULL;
    }

    return handle;
}

MutexHandle *open_mutex(const char *name)
{
    MutexHandle *handle = (MutexHandle *)malloc(sizeof(MutexHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for mutex handle");
        return NULL;
    }

    handle->handle = OpenMutex(
        SYNCHRONIZE, // Access right
        FALSE,       // Don't inherit
        (name)       // Name
    );

    if (handle->handle == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not open mutex (%lu).", GetLastError());
        error(errorMsg);
        free(handle);
        return NULL;
    }

    return handle;
}

bool lock_mutex(MutexHandle *handle)
{
    if (handle == NULL || handle->handle == NULL)
    {
        return 0;
    }

    DWORD result = WaitForSingleObject(handle->handle, INFINITE);
    return result == WAIT_OBJECT_0;
}

bool unlock_mutex(MutexHandle *handle)
{
    if (handle == NULL || handle->handle == NULL)
    {
        return 0;
    }

    return ReleaseMutex(handle->handle) != 0;
}

bool close_mutex(MutexHandle *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    bool result = CloseHandle(handle->handle) != 0;
    free(handle);
    return result;
}

// Semaphore operations
SemaphoreHandle *create_semaphore(const char *name, long initial_count, long max_count)
{
    SemaphoreHandle *handle = (SemaphoreHandle *)malloc(sizeof(SemaphoreHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for semaphore handle");
        return NULL;
    }

    handle->handle = CreateSemaphore(
        NULL,          // Default security attributes
        initial_count, // Initial count
        max_count,     // Maximum count
        (name)         // Named semaphore
    );

    if (handle->handle == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not create semaphore (%lu).", GetLastError());
        error(errorMsg);
        free(handle);
        return NULL;
    }

    return handle;
}

SemaphoreHandle *open_semaphore(const char *name)
{
    SemaphoreHandle *handle = (SemaphoreHandle *)malloc(sizeof(SemaphoreHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for semaphore handle");
        return NULL;
    }

    handle->handle = OpenSemaphore(
        SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, // Rights
        FALSE,                                // Don't inherit
        (name)                                // Name
    );

    if (handle->handle == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not open semaphore (%lu).", GetLastError());
        error(errorMsg);
        free(handle);
        return NULL;
    }

    return handle;
}

bool wait_semaphore(SemaphoreHandle *handle)
{
    if (handle == NULL || handle->handle == NULL)
    {
        return 0;
    }

    DWORD result = WaitForSingleObject(handle->handle, INFINITE);
    return result == WAIT_OBJECT_0;
}

bool release_semaphore(SemaphoreHandle *handle, long release_count)
{
    if (handle == NULL || handle->handle == NULL)
    {
        return 0;
    }

    return ReleaseSemaphore(handle->handle, release_count, NULL) != 0;
}

bool close_semaphore(SemaphoreHandle *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    bool result = CloseHandle(handle->handle) != 0;
    free(handle);
    return result;
}

// Platform-independent wait function
void platform_sleep(unsigned int milliseconds)
{
    Sleep(milliseconds);
}

// Platform-independent keyboard input functions
int getch()
{
    return _getch();
}

#endif // _WIN32