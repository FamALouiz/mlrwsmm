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
    int readerId = 1; // Default reader ID

    // If we have a command line argument, use it as the reader ID
    if (argc > 1)
    {
        readerId = atoi(argv[1]);
    }

    // Initialize the logger with terminal output and INFO verbosity
    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    // Set random seed
    srand((unsigned int)time(NULL) + readerId);

    // Open the existing shared memory
    hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,     // Read/write access
        FALSE,                   // Do not inherit the name
        TEXT(SHARED_MEMORY_NAME) // Name
    );

    if (hMapFile == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Reader %d: Could not open file mapping object (%lu).", readerId, GetLastError());
        error(errorMsg);

        char warningMsg[100];
        sprintf(warningMsg, "Reader %d: Make sure a writer is running first to create the shared memory.", readerId);
        warn(warningMsg);

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
        sprintf(errorMsg, "Reader %d: Could not map view of file (%lu).", readerId, GetLastError());
        error(errorMsg);
        CloseHandle(hMapFile);
        close_logger();
        return 1;
    }

    // Open synchronization objects
    mutexHandle = OpenMutex(
        SYNCHRONIZE,     // Access right
        FALSE,           // Don't inherit
        TEXT(MUTEX_NAME) // Name
    );

    writerSemaphore = OpenSemaphore(
        SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, // Rights
        FALSE,                                // Don't inherit
        TEXT(WRITER_SEMAPHORE_NAME)           // Name
    );

    readerSemaphore = OpenSemaphore(
        SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, // Rights
        FALSE,                                // Don't inherit
        TEXT(READER_SEMAPHORE_NAME)           // Name
    );

    priorityMutex = OpenMutex(
        SYNCHRONIZE,              // Access right
        FALSE,                    // Don't inherit
        TEXT(PRIORITY_MUTEX_NAME) // Name
    );

    // Check if all handles were opened successfully
    if (mutexHandle == NULL || writerSemaphore == NULL || readerSemaphore == NULL || priorityMutex == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Reader %d: Failed to open synchronization objects (%lu).", readerId, GetLastError());
        error(errorMsg);
        cleanup();
        close_logger();
        return 1;
    }

    char startMsg[100];
    sprintf(startMsg, "Reader %d: Starting. Press 'q' to quit, 'p' to toggle priority mode.", readerId);
    info(startMsg);

    int lastMessageId = -1; // Track the last message read
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
                sprintf(priorityMsg, "Reader %d: Priority mode switched to %s priority.",
                        readerId,
                        sharedData->isPriorityWriter ? "writer" : "reader");
                info(priorityMsg);

                ReleaseMutex(priorityMutex);
                Sleep(1000); // Prevent rapid toggling
            }
        }

        // Reader logic with priority handling
        WaitForSingleObject(mutexHandle, INFINITE);
        sharedData->waitingReaders++;

        if (sharedData->isPriorityWriter && sharedData->writerCount > 0)
        {
            // Writer priority and a writer is active
            char waitMsg[100];
            sprintf(waitMsg, "Reader %d: Writer priority is active, waiting for writers to finish...", readerId);
            info(waitMsg);
            ReleaseMutex(mutexHandle);
            Sleep(500); // Small wait before trying again
            continue;
        }

        // Reader can proceed, update counts
        sharedData->waitingReaders--;
        sharedData->readerCount++;

        // If this is the first reader, block writers
        if (sharedData->readerCount == 1)
        {
            // In writer priority mode, we need to explicitly acquire the semaphore
            if (sharedData->isPriorityWriter)
            {
                ReleaseMutex(mutexHandle);
                WaitForSingleObject(writerSemaphore, INFINITE);
                WaitForSingleObject(mutexHandle, INFINITE);
            }
        }

        ReleaseMutex(mutexHandle);

        // Read the shared memory
        if (lastMessageId != sharedData->messageId)
        {
            char readMsg[256];
            sprintf(readMsg, "Reader %d: Read message: %s", readerId, sharedData->message);
            info(readMsg);
            lastMessageId = sharedData->messageId;
        }
        else
        {
            char noNewMsg[100];
            sprintf(noNewMsg, "Reader %d: No new messages.", readerId);
            info(noNewMsg);
        }

        // Simulate reading work
        Sleep(rand() % 1500 + 500);

        // Done reading, update the shared memory
        WaitForSingleObject(mutexHandle, INFINITE);
        sharedData->readerCount--;

        // If this is the last reader, release the writer semaphore
        if (sharedData->readerCount == 0)
        {
            if (sharedData->isPriorityWriter)
            {
                ReleaseSemaphore(writerSemaphore, 1, NULL);
            }
            else
            {
                // For reader priority, ensure reader semaphore is released
                ReleaseSemaphore(readerSemaphore, 1, NULL);
            }
        }

        ReleaseMutex(mutexHandle);

        // Random sleep between read operations
        Sleep(rand() % 2000 + 500);
    }

    char terminateMsg[100];
    sprintf(terminateMsg, "Reader %d: Terminating.", readerId);
    info(terminateMsg);
    cleanup();

    // Close the logger before exiting
    close_logger();
    return 0;
}
