#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "..\..\include\common.h"
#include "..\..\include\log\logger.h"

// Global handles for synchronization
HANDLE hMapFile;
HANDLE mutexHandle;
HANDLE writerSemaphore;
HANDLE readerSemaphore;
HANDLE priorityMutex;
SharedData *sharedData;

void cleanup()
{
    if (sharedData)
        UnmapViewOfFile(sharedData);
    if (hMapFile)
        CloseHandle(hMapFile);
    if (mutexHandle)
        CloseHandle(mutexHandle);
    if (writerSemaphore)
        CloseHandle(writerSemaphore);
    if (readerSemaphore)
        CloseHandle(readerSemaphore);
    if (priorityMutex)
        CloseHandle(priorityMutex);
}

int main(int argc, char *argv[])
{
    int writerId = 1; // Default writer ID

    // If we have a command line argument, use it as the writer ID
    if (argc > 1)
    {
        writerId = atoi(argv[1]);
    }

    // Initialize the logger with terminal output and INFO verbosity
    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    // Set random seed for message generation
    srand((unsigned int)time(NULL) + writerId);

    // Create or open the shared memory
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // Not using a file
        NULL,                    // Default security attributes
        PAGE_READWRITE,          // Read/write access
        0,                       // High-order size
        SHARED_MEM_SIZE,         // Low-order size
        TEXT(SHARED_MEMORY_NAME) // Name
    );

    if (hMapFile == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Writer %d: Could not create file mapping object (%lu).", writerId, GetLastError());
        error(errorMsg);
        close_logger();
        return 1;
    }

    // Map view of the shared memory
    sharedData = (SharedData *)MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEM_SIZE);

    if (sharedData == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Writer %d: Could not map view of file (%lu).", writerId, GetLastError());
        error(errorMsg);
        CloseHandle(hMapFile);
        close_logger();
        return 1;
    }

    // Create synchronization objects
    mutexHandle = CreateMutex(
        NULL,            // Default security
        FALSE,           // Initially not owned
        TEXT(MUTEX_NAME) // Name
    );

    writerSemaphore = CreateSemaphore(
        NULL,
        1,                          // Initial count (1 = free for first writer)
        1,                          // Max count (only one writer at a time)
        TEXT(WRITER_SEMAPHORE_NAME) // Name
    );

    readerSemaphore = CreateSemaphore(
        NULL,
        1,                          // Initial count
        1,                          // Max count
        TEXT(READER_SEMAPHORE_NAME) // Name
    );

    priorityMutex = CreateMutex(
        NULL,                     // Default security
        FALSE,                    // Initially not owned
        TEXT(PRIORITY_MUTEX_NAME) // Name
    );

    // Initialize shared memory if this is the first writer
    DWORD result = WaitForSingleObject(mutexHandle, INFINITE);
    if (result == WAIT_OBJECT_0)
    {
        if (sharedData->messageId == 0)
        { // Initial state
            sharedData->readerCount = 0;
            sharedData->writerCount = 0;
            sharedData->waitingWriters = 0;
            sharedData->waitingReaders = 0;
            sharedData->isPriorityWriter = 1; // Default to writer priority
            sharedData->messageId = 0;
            strcpy_s(sharedData->message, 256, "Shared memory initialized");

            char initMsg[100];
            sprintf(initMsg, "Writer %d: Initialized shared memory.", writerId);
            info(initMsg);
        }
        ReleaseMutex(mutexHandle);
    }
    else
    {
        char errorMsg[100];
        sprintf(errorMsg, "Writer %d: Failed to acquire mutex for initialization (%lu).", writerId, GetLastError());
        error(errorMsg);
        cleanup();
        close_logger();
        return 1;
    }

    char startMsg[100];
    sprintf(startMsg, "Writer %d: Starting. Press 'q' to quit, 'p' to toggle priority mode.", writerId);
    info(startMsg);

    BOOL running = TRUE;
    while (running)
    {
        char input = 0;

        // Non-blocking check for keyboard input
        if (_kbhit())
        {
            input = _getch();
            if (input == 'q' || input == 'Q')
            {
                running = FALSE;
                continue;
            }
            else if (input == 'p' || input == 'P')
            {
                // Toggle priority mode
                WaitForSingleObject(priorityMutex, INFINITE);
                sharedData->isPriorityWriter = !sharedData->isPriorityWriter;

                char priorityMsg[100];
                sprintf(priorityMsg, "Writer %d: Priority mode switched to %s priority.",
                        writerId,
                        sharedData->isPriorityWriter ? "writer" : "reader");
                info(priorityMsg);

                ReleaseMutex(priorityMutex);
                Sleep(1000); // Prevent rapid toggling
            }
        }

        // Writer logic with priority handling
        WaitForSingleObject(mutexHandle, INFINITE);
        sharedData->waitingWriters++;
        ReleaseMutex(mutexHandle);

        char waitMsg[100];
        sprintf(waitMsg, "Writer %d: Waiting to write...", writerId);
        info(waitMsg);

        // Wait for access based on the priority rules
        if (sharedData->isPriorityWriter)
        {
            // Writer priority: wait for the writer semaphore only
            WaitForSingleObject(writerSemaphore, INFINITE);
        }
        else
        {
            // Reader priority: wait for both semaphores
            WaitForSingleObject(readerSemaphore, INFINITE);
            WaitForSingleObject(writerSemaphore, INFINITE);
        }

        // Got access, update waiting count
        WaitForSingleObject(mutexHandle, INFINITE);
        sharedData->waitingWriters--;
        sharedData->writerCount++;
        ReleaseMutex(mutexHandle);

        // Write to shared memory
        sharedData->messageId++;
        sprintf_s(sharedData->message, 256, "Message #%d from Writer %d at %lu",
                  sharedData->messageId, writerId, GetTickCount());

        char writeMsg[256];
        sprintf(writeMsg, "Writer %d: Wrote message: %s", writerId, sharedData->message);
        info(writeMsg);

        // Simulate writing work
        Sleep(rand() % 2000 + 1000);

        // Release access
        WaitForSingleObject(mutexHandle, INFINITE);
        sharedData->writerCount--;

        if (!sharedData->isPriorityWriter)
        {
            // For reader priority, release the reader semaphore
            ReleaseSemaphore(readerSemaphore, 1, NULL);
        }

        ReleaseMutex(mutexHandle);
        ReleaseSemaphore(writerSemaphore, 1, NULL);

        // Random sleep between write operations
        Sleep(rand() % 3000 + 1000);
    }

    char terminateMsg[100];
    sprintf(terminateMsg, "Writer %d: Terminating.", writerId);
    info(terminateMsg);
    cleanup();

    // Close the logger before exiting
    close_logger();
    return 0;
}