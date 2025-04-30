#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

const char *messageTemplates[] = {
    "Hello from Writer %d! This is message #%d.",
    "Writer %d checking in with update #%d.",
    "Breaking news from Writer %d: Message #%d has arrived!",
    "Writer %d strikes again with message #%d!",
    "This is Writer %d broadcasting message #%d."};

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

void generateMessage(int writerId, int messageCount, char *buffer, int bufferSize)
{
    int templateIndex = rand() % (sizeof(messageTemplates) / sizeof(messageTemplates[0]));
    snprintf(buffer, bufferSize, messageTemplates[templateIndex], writerId, messageCount);
}

int main(int argc, char *argv[])
{
    int writerId = 1;

    if (argc > 1)
    {
        writerId = atoi(argv[1]);
    }

    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    srand((unsigned int)time(NULL) + writerId);

    char startMsg[100];
    sprintf(startMsg, "Writer %d: Starting. Press 'q' to quit, 'p' to toggle priority mode.", writerId);
    info(startMsg);

    sharedMemoryHandle = open_shared_memory(SHARED_MEMORY_NAME);
    bool isFirstWriter = 0;

    if (sharedMemoryHandle == NULL)
    {
        info("Creating shared memory...");
        sharedMemoryHandle = create_shared_memory(SHARED_MEMORY_NAME, SHARED_MEM_SIZE);
        isFirstWriter = 1;

        if (sharedMemoryHandle == NULL)
        {
            error("Could not create shared memory. Exiting.");
            close_logger();
            return 1;
        }
    }

    sharedData = (SharedData *)map_shared_memory(sharedMemoryHandle, SHARED_MEM_SIZE);

    if (sharedData == NULL)
    {
        error("Could not map shared memory. Exiting.");
        close_shared_memory(sharedMemoryHandle);
        close_logger();
        return 1;
    }

    if (isFirstWriter)
    {
        info("Initializing shared memory and creating synchronization objects...");

        memset(sharedData, 0, SHARED_MEM_SIZE);
        sharedData->isPriorityWriter = 0;
        strcpy(sharedData->message, "Initial message");
        sharedData->messageId = 0;

        mutexHandle = create_mutex(MUTEX_NAME);
        writerSemaphore = create_semaphore(WRITER_SEMAPHORE_NAME, 1, 1);
        readerSemaphore = create_semaphore(READER_SEMAPHORE_NAME, 1, 1);
        priorityMutex = create_mutex(PRIORITY_MUTEX_NAME);
    }
    else
    {
        mutexHandle = open_mutex(MUTEX_NAME);
        writerSemaphore = open_semaphore(WRITER_SEMAPHORE_NAME);
        readerSemaphore = open_semaphore(READER_SEMAPHORE_NAME);
        priorityMutex = open_mutex(PRIORITY_MUTEX_NAME);
    }

    if (mutexHandle == NULL || writerSemaphore == NULL || readerSemaphore == NULL || priorityMutex == NULL)
    {
        error("Failed to create or open synchronization objects. Exiting.");
        cleanup();
        close_logger();
        return 1;
    }

    int messageCount = 0;
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
                sprintf(priorityMsg, "Writer %d: Priority mode switched to %s priority.",
                        writerId,
                        sharedData->isPriorityWriter ? "writer" : "reader");
                info(priorityMsg);

                unlock_mutex(priorityMutex);
                platform_sleep(1000);
            }
        }

        lock_mutex(mutexHandle);
        sharedData->waitingWriters++;

        if (sharedData->readerCount > 0 && !sharedData->isPriorityWriter)
        {
            char waitMsg[100];
            sprintf(waitMsg, "Writer %d: Reader priority is active, waiting for readers to finish...", writerId);
            info(waitMsg);
            unlock_mutex(mutexHandle);
            platform_sleep(500);
            continue;
        }

        sharedData->waitingWriters--;
        sharedData->writerCount++;

        unlock_mutex(mutexHandle);

        char acquireMsg[100];
        sprintf(acquireMsg, "Writer %d: Acquiring exclusive write access...", writerId);
        info(acquireMsg);

        wait_semaphore(writerSemaphore);

        messageCount++;
        char newMessage[256];
        generateMessage(writerId, messageCount, newMessage, sizeof(newMessage));

        strcpy(sharedData->message, newMessage);
        sharedData->messageId++;

        char writeMsg[100];
        sprintf(writeMsg, "Writer %d: Writing message: %s", writerId, newMessage);
        info(writeMsg);
        platform_sleep(rand() % 1000 + 500);

        release_semaphore(writerSemaphore, 1);

        lock_mutex(mutexHandle);
        sharedData->writerCount--;

        if (sharedData->writerCount == 0 && !sharedData->isPriorityWriter)
        {
            release_semaphore(readerSemaphore, 1);
        }

        unlock_mutex(mutexHandle);

        platform_sleep(rand() % 3000 + 1000);
    }

    char terminateMsg[100];
    sprintf(terminateMsg, "Writer %d: Terminating.", writerId);
    info(terminateMsg);
    cleanup();

    close_logger();
    return 0;
}