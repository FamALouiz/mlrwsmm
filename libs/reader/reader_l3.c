#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../include/common.h"
#include "../../include/log/logger.h"
#include "../../include/platform/shared_memory.h"
#include "../../include/platform/sync.h"

// Global handles for Level 2
SharedMemoryHandle *sharedMemoryL2Handle;
MutexHandle *mutexL2Handle;
SemaphoreHandle *writerSemL2;
SemaphoreHandle *readerSemL2;
MutexHandle *priorityMutex;
SharedDataL2 *sharedDataL2;

void cleanup()
{
    if (sharedDataL2)
        unmap_shared_memory(sharedDataL2);
    if (sharedMemoryL2Handle)
        close_shared_memory(sharedMemoryL2Handle);
    if (mutexL2Handle)
        close_mutex(mutexL2Handle);
    if (writerSemL2)
        close_semaphore(writerSemL2);
    if (readerSemL2)
        close_semaphore(readerSemL2);
    if (priorityMutex)
        close_mutex(priorityMutex);
}

void processAggregatedData(int readerId)
{
    // Simulate different processing strategies for each reader
    char processMsg[200];
    time_t currentTime = time(NULL);

    switch (readerId)
    {
    case 1:
        // Reader 1: Focus on data freshness analysis
        {
            double timeDiff = difftime(currentTime, sharedDataL2->lastUpdateTime);
            sprintf(processMsg, "L3-Reader %d [FRESHNESS ANALYZER]: Data age: %.2f seconds, Messages: %d",
                    readerId, timeDiff, sharedDataL2->totalMessages);
            info(processMsg);

            if (timeDiff < 5.0)
            {
                info("L3-Reader 1: Data is FRESH - Real-time processing enabled");
            }
            else
            {
                info("L3-Reader 1: Data is STALE - Batch processing mode");
            }
        }
        break;

    case 2:
        // Reader 2: Focus on writer statistics
        {
            sprintf(processMsg, "L3-Reader %d [STATISTICS ANALYZER]: Processing writer performance data", readerId);
            info(processMsg);

            for (int i = 0; i < MAX_WRITERS_L1; i++)
            {
                if (sharedDataL2->messagesFromWriter[i] > 0)
                {
                    char writerStat[100];
                    sprintf(writerStat, "L3-Reader 2: Writer %d produced %d messages",
                            i + 1, sharedDataL2->messagesFromWriter[i]);
                    info(writerStat);
                }
            }

            // Calculate throughput
            double avgMessages = (double)sharedDataL2->totalMessages / MAX_WRITERS_L1;
            sprintf(processMsg, "L3-Reader 2: Average throughput per writer: %.2f messages", avgMessages);
            info(processMsg);
        }
        break;

    case 3:
        // Reader 3: Focus on content analysis
        {
            sprintf(processMsg, "L3-Reader %d [CONTENT ANALYZER]: Analyzing aggregated content", readerId);
            info(processMsg);

            // Count different message types in the aggregated data
            int highPriorityCount = 0;
            int activeCount = 0;
            int dataStreamCount = 0;

            char *content = sharedDataL2->aggregatedData;
            if (strstr(content, "Priority: HIGH"))
                highPriorityCount++;
            if (strstr(content, "Status: ACTIVE"))
                activeCount++;
            if (strstr(content, "Data stream"))
                dataStreamCount++;

            sprintf(processMsg, "L3-Reader 3: Found - High Priority: %d, Active: %d, Streams: %d",
                    highPriorityCount, activeCount, dataStreamCount);
            info(processMsg);
        }
        break;

    default:
        sprintf(processMsg, "L3-Reader %d [GENERAL]: Processing aggregated data from Level 2", readerId);
        info(processMsg);
    }
}

void displayAggregatedData(int readerId)
{
    char displayMsg[100];
    sprintf(displayMsg, "L3-Reader %d: === DISPLAYING AGGREGATED DATA ===", readerId);
    info(displayMsg);

    // Display key information (not the full content to avoid log spam)
    char infoMsg[200];
    sprintf(infoMsg, "L3-Reader %d: Total messages: %d, Last update: %s",
            readerId, sharedDataL2->totalMessages, ctime(&sharedDataL2->lastUpdateTime));
    info(infoMsg);

    // Display first few lines of aggregated data
    char *content = sharedDataL2->aggregatedData;
    char *line = strtok(content, "\n");
    int lineCount = 0;

    while (line != NULL && lineCount < 3)
    {
        char lineMsg[300];
        sprintf(lineMsg, "L3-Reader %d: %s", readerId, line);
        info(lineMsg);
        line = strtok(NULL, "\n");
        lineCount++;
    }

    if (lineCount >= 3)
    {
        sprintf(displayMsg, "L3-Reader %d: ... (truncated for brevity)", readerId);
        info(displayMsg);
    }
}

int main(int argc, char *argv[])
{
    int readerId = 1;

    if (argc > 1)
    {
        readerId = atoi(argv[1]);
        if (readerId < 1 || readerId > MAX_READERS_L3)
        {
            printf("Error: Reader ID must be between 1 and %d\n", MAX_READERS_L3);
            return 1;
        }
    }

    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);
    srand((unsigned int)time(NULL) + readerId);

    char startMsg[100];
    sprintf(startMsg, "L3-Reader %d: Starting Level 3 reader. Press 'q' to quit.", readerId);
    info(startMsg);

    // Open Level 2 shared memory (must exist)
    sharedMemoryL2Handle = open_shared_memory(SHARED_MEMORY_L2_NAME);
    if (sharedMemoryL2Handle == NULL)
    {
        error("Could not open Level 2 shared memory. Make sure the aggregator is running.");
        close_logger();
        return 1;
    }

    sharedDataL2 = (SharedDataL2 *)map_shared_memory(sharedMemoryL2Handle, SHARED_MEM_L2_SIZE);
    if (sharedDataL2 == NULL)
    {
        error("Could not map Level 2 shared memory.");
        close_shared_memory(sharedMemoryL2Handle);
        close_logger();
        return 1;
    }

    // Open synchronization objects
    mutexL2Handle = open_mutex(MUTEX_L2_NAME);
    writerSemL2 = open_semaphore(WRITER_SEM_L2_NAME);
    readerSemL2 = open_semaphore(READER_SEM_L2_NAME);
    priorityMutex = open_mutex(PRIORITY_MUTEX_NAME);

    if (mutexL2Handle == NULL || writerSemL2 == NULL || readerSemL2 == NULL || priorityMutex == NULL)
    {
        error("Failed to open Level 2 synchronization objects. Exiting.");
        cleanup();
        close_logger();
        return 1;
    }

    char registerMsg[100];
    sprintf(registerMsg, "L3-Reader %d: Successfully connected to Level 2 shared memory", readerId);
    info(registerMsg);

    bool running = true;
    int readCount = 0;
    int lastProcessedMessageCount = 0;

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

        // Wait for reader access (multiple readers can read simultaneously)
        wait_semaphore(readerSemL2);

        // Increment reader count
        lock_mutex(mutexL2Handle);
        sharedDataL2->readerCount++;

        // Check if there's new data to process
        bool hasNewData = (sharedDataL2->aggregatedMessageCount > lastProcessedMessageCount);
        int currentMessageCount = sharedDataL2->aggregatedMessageCount;
        unlock_mutex(mutexL2Handle);

        if (hasNewData)
        {
            readCount++;

            char readMsg[100];
            sprintf(readMsg, "L3-Reader %d: Reading aggregated data (Read #%d)", readerId, readCount);
            info(readMsg);

            // Process the aggregated data
            processAggregatedData(readerId);

            // Optionally display data (controlled to avoid spam)
            if (readCount % 3 == 0)
            { // Display every 3rd read
                displayAggregatedData(readerId);
            }

            lastProcessedMessageCount = currentMessageCount;

            // Simulate processing time
            platform_sleep(rand() % 1000 + 500);
        }
        else
        {
            char waitMsg[100];
            sprintf(waitMsg, "L3-Reader %d: No new data, waiting...", readerId);
            info(waitMsg);
        }

        // Decrement reader count
        lock_mutex(mutexL2Handle);
        sharedDataL2->readerCount--;

        // If this was the last reader and writer priority is set, signal writers
        if (sharedDataL2->readerCount == 0)
        {
            // Check if any writers are waiting
            release_semaphore(writerSemL2, 1);
        }
        unlock_mutex(mutexL2Handle);

        release_semaphore(readerSemL2, 1);

        // Wait between reads
        platform_sleep(rand() % 2000 + 1000);
    }

    char terminateMsg[100];
    sprintf(terminateMsg, "L3-Reader %d: Terminating after %d read operations.", readerId, readCount);
    info(terminateMsg);

    cleanup();
    close_logger();
    return 0;
}
