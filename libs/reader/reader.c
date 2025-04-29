#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../include/common.h"
#include "../../include/log/logger.h"
#include "../../include/platform/shared_memory.h"
#include "../../include/platform/sync.h"

SharedMemoryHandle *sharedMemoryHandle;
MutexHandle *mutexHandle;
SemaphoreHandle *writerSemaphore;
SemaphoreHandle *readerSemaphore;
MutexHandle *priorityMutex;
SharedData *sharedData;

void cleanup()
{
    if (sharedData)
        unmap_shared_memory(sharedData);
    if (sharedMemoryHandle)
        close_shared_memory(sharedMemoryHandle);
    if (mutexHandle)
        close_mutex(mutexHandle);
    if (writerSemaphore)
        close_semaphore(writerSemaphore);
    if (readerSemaphore)
        close_semaphore(readerSemaphore);
    if (priorityMutex)
        close_mutex(priorityMutex);
}

int main(int argc, char *argv[])
{
    int readerId = 1;

    if (argc > 1)
    {
        readerId = atoi(argv[1]);
    }

    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    srand((unsigned int)time(NULL) + readerId);

    sharedMemoryHandle = open_shared_memory(SHARED_MEMORY_NAME);

    if (sharedMemoryHandle == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Reader %d: Could not open shared memory.", readerId);
        error(errorMsg);

        char warningMsg[100];
        sprintf(warningMsg, "Reader %d: Make sure a writer is running first to create the shared memory.", readerId);
        warn(warningMsg);

        close_logger();
        return 1;
    }

    sharedData = (SharedData *)map_shared_memory(sharedMemoryHandle, SHARED_MEM_SIZE);

    if (sharedData == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Reader %d: Could not map shared memory.", readerId);
        error(errorMsg);
        close_shared_memory(sharedMemoryHandle);
        close_logger();
        return 1;
    }

    mutexHandle = open_mutex(MUTEX_NAME);
    writerSemaphore = open_semaphore(WRITER_SEMAPHORE_NAME);
    readerSemaphore = open_semaphore(READER_SEMAPHORE_NAME);
    priorityMutex = open_mutex(PRIORITY_MUTEX_NAME);

    if (mutexHandle == NULL || writerSemaphore == NULL || readerSemaphore == NULL || priorityMutex == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Reader %d: Failed to open synchronization objects.", readerId);
        error(errorMsg);
        cleanup();
        close_logger();
        return 1;
    }

    char startMsg[100];
    sprintf(startMsg, "Reader %d: Starting. Press 'q' to quit, 'p' to toggle priority mode.", readerId);
    info(startMsg);

    int lastMessageId = -1;
    bool running = 1;

    while (running)
    {
        char input = 0;

        if (kbhit())
        {
            input = getch();
            if (input == 'q' || input == 'Q')
            {
                running = 0;
                continue;
            }
            else if (input == 'p' || input == 'P')
            {
                lock_mutex(priorityMutex);
                sharedData->isPriorityWriter = !sharedData->isPriorityWriter;

                char priorityMsg[100];
                sprintf(priorityMsg, "Reader %d: Priority mode switched to %s priority.",
                        readerId,
                        sharedData->isPriorityWriter ? "writer" : "reader");
                info(priorityMsg);

                unlock_mutex(priorityMutex);
                platform_sleep(1000);
            }
        }

        lock_mutex(mutexHandle);
        sharedData->waitingReaders++;

        if (sharedData->isPriorityWriter && sharedData->writerCount > 0)
        {
            char waitMsg[100];
            sprintf(waitMsg, "Reader %d: Writer priority is active, waiting for writers to finish...", readerId);
            info(waitMsg);
            unlock_mutex(mutexHandle);
            platform_sleep(500);
            continue;
        }

        sharedData->waitingReaders--;
        sharedData->readerCount++;

        if (sharedData->readerCount == 1)
        {
            if (sharedData->isPriorityWriter)
            {
                unlock_mutex(mutexHandle);
                wait_semaphore(writerSemaphore);
                lock_mutex(mutexHandle);
            }
        }

        unlock_mutex(mutexHandle);

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

        platform_sleep(rand() % 1500 + 500);

        lock_mutex(mutexHandle);
        sharedData->readerCount--;

        if (sharedData->readerCount == 0)
        {
            if (sharedData->isPriorityWriter)
            {
                release_semaphore(writerSemaphore, 1);
            }
            else
            {
                release_semaphore(readerSemaphore, 1);
            }
        }

        unlock_mutex(mutexHandle);

        platform_sleep(rand() % 2000 + 500);
    }

    char terminateMsg[100];
    sprintf(terminateMsg, "Reader %d: Terminating.", readerId);
    info(terminateMsg);
    cleanup();

    close_logger();
    return 0;
}
