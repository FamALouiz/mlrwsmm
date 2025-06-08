#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../include/common.h"
#include "../../include/log/logger.h"
#include "../../include/platform/shared_memory.h"
#include "../../include/platform/sync.h"

// Global handles for both levels
SharedMemoryHandle *sharedMemoryL1Handle;
SharedMemoryHandle *sharedMemoryL2Handle;
MutexHandle *mutexL1Handle;
MutexHandle *mutexL2Handle;
SemaphoreHandle *readerSemL1;
SemaphoreHandle *writerSemL2;
SemaphoreHandle *readerSemL2;
MutexHandle *priorityMutex;
SemaphoreHandle *aggregatorSignal;

SharedDataL1 *sharedDataL1;
SharedDataL2 *sharedDataL2;

void cleanup()
{
    if (sharedDataL1)
        unmap_shared_memory(sharedDataL1);
    if (sharedDataL2)
        unmap_shared_memory(sharedDataL2);
    if (sharedMemoryL1Handle)
        close_shared_memory(sharedMemoryL1Handle);
    if (sharedMemoryL2Handle)
        close_shared_memory(sharedMemoryL2Handle);
    if (mutexL1Handle)
        close_mutex(mutexL1Handle);
    if (mutexL2Handle)
        close_mutex(mutexL2Handle);
    if (readerSemL1)
        close_semaphore(readerSemL1);
    if (writerSemL2)
        close_semaphore(writerSemL2);
    if (readerSemL2)
        close_semaphore(readerSemL2);
    if (priorityMutex)
        close_mutex(priorityMutex);
    if (aggregatorSignal)
        close_semaphore(aggregatorSignal);
}

void aggregateData()
{
    char tempBuffer[MAX_AGGREGATED_SIZE];
    memset(tempBuffer, 0, MAX_AGGREGATED_SIZE);

    time_t currentTime = time(NULL);
    double totalTimestamp = 0;
    int validMessages = 0;

    // Read from Level 1 (protected by reader access)
    wait_semaphore(readerSemL1);

    lock_mutex(mutexL1Handle);
    sharedDataL1->readerCount++;
    unlock_mutex(mutexL1Handle);

    // Build aggregated message header
    sprintf(tempBuffer, "=== AGGREGATED DATA REPORT ===\n");
    sprintf(tempBuffer + strlen(tempBuffer), "Timestamp: %s", ctime(&currentTime));
    sprintf(tempBuffer + strlen(tempBuffer), "Active Writers: %d\n", sharedDataL1->activeWriters);
    sprintf(tempBuffer + strlen(tempBuffer), "Total Messages Processed: %d\n\n", sharedDataL1->messageCount);

    // Process each writer's data
    for (int i = 0; i < MAX_WRITERS_L1; i++)
    {
        if (sharedDataL1->writerData[i].isActive)
        {
            sprintf(tempBuffer + strlen(tempBuffer),
                    "Writer %d [Slot %d]: %s (ID: %d, Time: %s)\n",
                    sharedDataL1->writerData[i].writerId,
                    i,
                    sharedDataL1->writerData[i].message,
                    sharedDataL1->writerData[i].messageId,
                    ctime(&sharedDataL1->writerData[i].timestamp));

            totalTimestamp += (double)sharedDataL1->writerData[i].timestamp;
            validMessages++;
        }
    }

    // Calculate statistics
    double avgTimestamp = validMessages > 0 ? totalTimestamp / validMessages : 0;

    sprintf(tempBuffer + strlen(tempBuffer), "\n=== STATISTICS ===\n");
    sprintf(tempBuffer + strlen(tempBuffer), "Valid Messages: %d\n", validMessages);
    sprintf(tempBuffer + strlen(tempBuffer), "Average Timestamp: %.2f\n", avgTimestamp);
    sprintf(tempBuffer + strlen(tempBuffer), "Data Freshness: %.2f seconds ago\n",
            difftime(currentTime, (time_t)avgTimestamp));
    sprintf(tempBuffer + strlen(tempBuffer), "=== END REPORT ===\n");

    // Copy writer statistics
    int writerStats[MAX_WRITERS_L1];
    for (int i = 0; i < MAX_WRITERS_L1; i++)
    {
        writerStats[i] = sharedDataL1->writerData[i].messageId;
    }

    lock_mutex(mutexL1Handle);
    sharedDataL1->readerCount--;
    unlock_mutex(mutexL1Handle);

    release_semaphore(readerSemL1, 1);

    // Write to Level 2 (exclusive writer access)
    wait_semaphore(writerSemL2);

    lock_mutex(mutexL2Handle);
    sharedDataL2->writerCount++;
    unlock_mutex(mutexL2Handle);

    // Update Level 2 shared data
    lock_mutex(mutexL2Handle);
    strcpy(sharedDataL2->aggregatedData, tempBuffer);
    sharedDataL2->totalMessages = sharedDataL1->messageCount;
    sharedDataL2->lastUpdateTime = currentTime;
    sharedDataL2->averageTimestamp = avgTimestamp;
    sharedDataL2->aggregatedMessageCount++;

    // Copy writer statistics
    for (int i = 0; i < MAX_WRITERS_L1; i++)
    {
        sharedDataL2->messagesFromWriter[i] = writerStats[i];
    }
    unlock_mutex(mutexL2Handle);

    info("Aggregator: Data aggregated and written to Level 2");

    lock_mutex(mutexL2Handle);
    sharedDataL2->writerCount--;

    // Signal Level 3 readers that new data is available
    if (sharedDataL2->writerCount == 0)
    {
        release_semaphore(readerSemL2, 1);
    }
    unlock_mutex(mutexL2Handle);

    release_semaphore(writerSemL2, 1);
}

int main(int argc, char *argv[])
{
    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    info("L2-Aggregator: Starting multi-level aggregation system. Press 'q' to quit.");

    // Open Level 1 shared memory (must exist)
    sharedMemoryL1Handle = open_shared_memory(SHARED_MEMORY_L1_NAME);
    if (sharedMemoryL1Handle == NULL)
    {
        error("Could not open Level 1 shared memory. Make sure Level 1 writers are running.");
        close_logger();
        return 1;
    }

    sharedDataL1 = (SharedDataL1 *)map_shared_memory(sharedMemoryL1Handle, SHARED_MEM_L1_SIZE);
    if (sharedDataL1 == NULL)
    {
        error("Could not map Level 1 shared memory.");
        close_shared_memory(sharedMemoryL1Handle);
        close_logger();
        return 1;
    }

    // Create Level 2 shared memory
    sharedMemoryL2Handle = create_shared_memory(SHARED_MEMORY_L2_NAME, SHARED_MEM_L2_SIZE);
    if (sharedMemoryL2Handle == NULL)
    {
        // Try to open existing
        sharedMemoryL2Handle = open_shared_memory(SHARED_MEMORY_L2_NAME);
        if (sharedMemoryL2Handle == NULL)
        {
            error("Could not create or open Level 2 shared memory.");
            cleanup();
            close_logger();
            return 1;
        }
    }

    sharedDataL2 = (SharedDataL2 *)map_shared_memory(sharedMemoryL2Handle, SHARED_MEM_L2_SIZE);
    if (sharedDataL2 == NULL)
    {
        error("Could not map Level 2 shared memory.");
        cleanup();
        close_logger();
        return 1;
    }

    // Initialize Level 2 data
    memset(sharedDataL2, 0, SHARED_MEM_L2_SIZE);
    sharedDataL2->aggregatedMessageCount = 0;
    strcpy(sharedDataL2->aggregatedData, "Aggregator starting...");

    // Open synchronization objects
    mutexL1Handle = open_mutex(MUTEX_L1_NAME);
    readerSemL1 = open_semaphore(READER_SEM_L1_NAME);
    priorityMutex = open_mutex(PRIORITY_MUTEX_NAME);

    // Create Level 2 synchronization objects
    mutexL2Handle = create_mutex(MUTEX_L2_NAME);
    writerSemL2 = create_semaphore(WRITER_SEM_L2_NAME, 1, 1);
    readerSemL2 = create_semaphore(READER_SEM_L2_NAME, 1, 1);
    aggregatorSignal = create_semaphore(AGGREGATOR_SIGNAL_NAME, 0, 1);

    if (mutexL1Handle == NULL || readerSemL1 == NULL || priorityMutex == NULL ||
        mutexL2Handle == NULL || writerSemL2 == NULL || readerSemL2 == NULL)
    {
        error("Failed to open/create synchronization objects.");
        cleanup();
        close_logger();
        return 1;
    }

    info("L2-Aggregator: Successfully initialized. Starting aggregation loop...");

    bool running = true;
    int aggregationCount = 0;

    while (running)
    {
        char input = 0;

        // Check for keyboard input
        if (kbhit())
        {
            input = getch();
            if (input == 'q' || input == 'Q')
            {
                running = false;
                continue;
            }
        }

        // Perform aggregation
        aggregationCount++;

        char aggMsg[100];
        sprintf(aggMsg, "L2-Aggregator: Starting aggregation cycle #%d", aggregationCount);
        info(aggMsg);

        aggregateData();

        char completeMsg[100];
        sprintf(completeMsg, "L2-Aggregator: Completed aggregation cycle #%d", aggregationCount);
        info(completeMsg);

        // Wait before next aggregation (configurable interval)
        platform_sleep(2000); // 2 seconds between aggregations
    }

    char terminateMsg[100];
    sprintf(terminateMsg, "L2-Aggregator: Terminating after %d aggregation cycles.", aggregationCount);
    info(terminateMsg);

    cleanup();
    close_logger();
    return 0;
}
