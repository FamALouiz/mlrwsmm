#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../include/common.h"
#include "../../include/log/logger.h"
#include "../../include/platform/shared_memory.h"
#include "../../include/platform/sync.h"

// Global handles for Level 1
SharedMemoryHandle *sharedMemoryL1Handle;
MutexHandle *mutexL1Handle;
SemaphoreHandle *writerSemL1;
SemaphoreHandle *readerSemL1;
MutexHandle *priorityMutex;
SharedDataL1 *sharedDataL1;

const char *messageTemplates[] = {
    "L1-Writer %d: Critical system data #%d - Priority: HIGH",
    "L1-Writer %d: Processing batch #%d - Status: ACTIVE",
    "L1-Writer %d: Data stream #%d - Throughput: %d MB/s",
    "L1-Writer %d: Transaction #%d - Response time: %d ms",
    "L1-Writer %d: Sensor reading #%d - Value: %.2f units"};

void cleanup()
{
    if (sharedDataL1)
        unmap_shared_memory(sharedDataL1);
    if (sharedMemoryL1Handle)
        close_shared_memory(sharedMemoryL1Handle);
    if (mutexL1Handle)
        close_mutex(mutexL1Handle);
    if (writerSemL1)
        close_semaphore(writerSemL1);
    if (readerSemL1)
        close_semaphore(readerSemL1);
    if (priorityMutex)
        close_mutex(priorityMutex);
}

void generateMessage(int writerId, int messageCount, char *buffer, int bufferSize)
{
    int templateIndex = rand() % (sizeof(messageTemplates) / sizeof(messageTemplates[0]));
    int randomValue = rand() % 1000 + 1;
    double randomFloat = (double)(rand() % 10000) / 100.0;

    switch (templateIndex)
    {
    case 2:
        snprintf(buffer, bufferSize, messageTemplates[templateIndex], writerId, messageCount, randomValue);
        break;
    case 3:
        snprintf(buffer, bufferSize, messageTemplates[templateIndex], writerId, messageCount, randomValue);
        break;
    case 4:
        snprintf(buffer, bufferSize, messageTemplates[templateIndex], writerId, messageCount, randomFloat);
        break;
    default:
        snprintf(buffer, bufferSize, messageTemplates[templateIndex], writerId, messageCount);
    }
}

int main(int argc, char *argv[])
{
    int writerId = 1;

    if (argc > 1)
    {
        writerId = atoi(argv[1]);
        if (writerId < 1 || writerId > MAX_WRITERS_L1)
        {
            printf("Error: Writer ID must be between 1 and %d\n", MAX_WRITERS_L1);
            return 1;
        }
    }

    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);
    srand((unsigned int)time(NULL) + writerId);

    char startMsg[100];
    sprintf(startMsg, "L1-Writer %d: Starting multi-level system. Press 'q' to quit, 'p' to toggle priority.", writerId);
    info(startMsg);

    // Try to open existing shared memory first
    sharedMemoryL1Handle = open_shared_memory(SHARED_MEMORY_L1_NAME);
    bool isFirstWriter = false;

    if (sharedMemoryL1Handle == NULL)
    {
        info("Creating Level 1 shared memory...");
        sharedMemoryL1Handle = create_shared_memory(SHARED_MEMORY_L1_NAME, SHARED_MEM_L1_SIZE);
        isFirstWriter = true;

        if (sharedMemoryL1Handle == NULL)
        {
            error("Could not create Level 1 shared memory. Exiting.");
            close_logger();
            return 1;
        }
    }

    sharedDataL1 = (SharedDataL1 *)map_shared_memory(sharedMemoryL1Handle, SHARED_MEM_L1_SIZE);
    if (sharedDataL1 == NULL)
    {
        error("Could not map Level 1 shared memory. Exiting.");
        close_shared_memory(sharedMemoryL1Handle);
        close_logger();
        return 1;
    }

    // Initialize synchronization objects
    if (isFirstWriter)
    {
        info("Initializing Level 1 shared memory and synchronization objects...");

        memset(sharedDataL1, 0, SHARED_MEM_L1_SIZE);
        sharedDataL1->isPriorityWriter = 0;
        sharedDataL1->messageCount = 0;
        sharedDataL1->activeWriters = 0;

        // Initialize all writer slots
        for (int i = 0; i < MAX_WRITERS_L1; i++)
        {
            sharedDataL1->writerData[i].writerId = 0;
            sharedDataL1->writerData[i].messageId = 0;
            sharedDataL1->writerData[i].isActive = 0;
            strcpy(sharedDataL1->writerData[i].message, "Uninitialized");
        }

        mutexL1Handle = create_mutex(MUTEX_L1_NAME);
        writerSemL1 = create_semaphore(WRITER_SEM_L1_NAME, MAX_WRITERS_L1, MAX_WRITERS_L1);
        readerSemL1 = create_semaphore(READER_SEM_L1_NAME, 1, 1);
        priorityMutex = create_mutex(PRIORITY_MUTEX_NAME);
    }
    else
    {
        mutexL1Handle = open_mutex(MUTEX_L1_NAME);
        writerSemL1 = open_semaphore(WRITER_SEM_L1_NAME);
        readerSemL1 = open_semaphore(READER_SEM_L1_NAME);
        priorityMutex = open_mutex(PRIORITY_MUTEX_NAME);
    }

    if (mutexL1Handle == NULL || writerSemL1 == NULL || readerSemL1 == NULL || priorityMutex == NULL)
    {
        error("Failed to create or open Level 1 synchronization objects. Exiting.");
        cleanup();
        close_logger();
        return 1;
    }

    int messageCount = 0;
    bool running = true;
    int writerSlot = writerId - 1; // Convert to 0-based index

    // Register this writer
    lock_mutex(mutexL1Handle);
    sharedDataL1->writerData[writerSlot].writerId = writerId;
    sharedDataL1->writerData[writerSlot].isActive = 1;
    sharedDataL1->activeWriters++;
    unlock_mutex(mutexL1Handle);

    char registerMsg[100];
    sprintf(registerMsg, "L1-Writer %d: Registered in slot %d", writerId, writerSlot);
    info(registerMsg);

    while (running)
    {
        char input = 0;

        // Check for keyboard input (non-blocking)
        if (kbhit())
        {
            input = getch();
            if (input == 'q' || input == 'Q')
            {
                running = false;
                continue;
            }
            else if (input == 'p' || input == 'P')
            {
                lock_mutex(priorityMutex);
                sharedDataL1->isPriorityWriter = !sharedDataL1->isPriorityWriter;

                char priorityMsg[100];
                sprintf(priorityMsg, "L1-Writer %d: Priority mode switched to %s priority.",
                        writerId, sharedDataL1->isPriorityWriter ? "writer" : "reader");
                info(priorityMsg);

                unlock_mutex(priorityMutex);
                platform_sleep(1000);
                continue;
            }
        }

        // Wait for semaphore (allows up to MAX_WRITERS_L1 concurrent writers)
        wait_semaphore(writerSemL1);

        // Check if readers are active and have priority
        lock_mutex(mutexL1Handle);
        if (sharedDataL1->readerCount > 0 && !sharedDataL1->isPriorityWriter)
        {
            char waitMsg[100];
            sprintf(waitMsg, "L1-Writer %d: Reader priority active, waiting...", writerId);
            info(waitMsg);
            unlock_mutex(mutexL1Handle);
            release_semaphore(writerSemL1, 1);
            platform_sleep(500);
            continue;
        }

        sharedDataL1->writerCount++;
        unlock_mutex(mutexL1Handle);

        // Generate and write message
        messageCount++;
        char newMessage[MAX_MESSAGE_SIZE];
        generateMessage(writerId, messageCount, newMessage, sizeof(newMessage));

        // Update this writer's data slot
        lock_mutex(mutexL1Handle);
        strcpy(sharedDataL1->writerData[writerSlot].message, newMessage);
        sharedDataL1->writerData[writerSlot].messageId = messageCount;
        sharedDataL1->writerData[writerSlot].timestamp = time(NULL);
        sharedDataL1->messageCount++;
        unlock_mutex(mutexL1Handle);

        char writeMsg[150];
        sprintf(writeMsg, "L1-Writer %d: [Slot %d] Writing: %s", writerId, writerSlot, newMessage);
        info(writeMsg);

        // Simulate write time
        platform_sleep(rand() % 1000 + 500);

        // Release writer access
        lock_mutex(mutexL1Handle);
        sharedDataL1->writerCount--;

        // If no more writers and reader priority, signal readers
        if (sharedDataL1->writerCount == 0 && !sharedDataL1->isPriorityWriter)
        {
            release_semaphore(readerSemL1, 1);
        }
        unlock_mutex(mutexL1Handle);

        release_semaphore(writerSemL1, 1);

        // Wait between writes
        platform_sleep(rand() % 3000 + 1000);
    }

    // Unregister this writer
    lock_mutex(mutexL1Handle);
    sharedDataL1->writerData[writerSlot].isActive = 0;
    sharedDataL1->activeWriters--;
    unlock_mutex(mutexL1Handle);

    char terminateMsg[100];
    sprintf(terminateMsg, "L1-Writer %d: Terminating after %d messages.", writerId, messageCount);
    info(terminateMsg);

    cleanup();
    close_logger();
    return 0;
}
