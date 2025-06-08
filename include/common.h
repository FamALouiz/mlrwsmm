#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <time.h>

// Multi-level shared memory and synchronization definitions
// Level 1: Writers to Shared Memory 1
#define SHARED_MEMORY_L1_NAME "RWSharedMemoryL1"
#define MUTEX_L1_NAME "RWMutexL1"
#define WRITER_SEM_L1_NAME "WriterSemL1"
#define READER_SEM_L1_NAME "ReaderSemL1"

// Level 2: Shared Memory 1 to Aggregator to Shared Memory 2
#define SHARED_MEMORY_L2_NAME "RWSharedMemoryL2"
#define MUTEX_L2_NAME "RWMutexL2"
#define WRITER_SEM_L2_NAME "WriterSemL2"
#define READER_SEM_L2_NAME "ReaderSemL2"

// Level 3: Global priority control
#define PRIORITY_MUTEX_NAME "PriorityMutex"
#define AGGREGATOR_SIGNAL_NAME "AggregatorSignal"

// Data structures for each level
#define MAX_WRITERS_L1 3
#define MAX_READERS_L3 3
#define MAX_MESSAGE_SIZE 256
#define MAX_AGGREGATED_SIZE 1024

// Level 1 shared data structure (written by 3 writers)
typedef struct
{
    int activeWriters;
    int waitingReaders;
    int messageCount;
    int isPriorityWriter;

    // Individual writer data
    struct
    {
        int writerId;
        char message[MAX_MESSAGE_SIZE];
        int messageId;
        time_t timestamp;
        int isActive;
    } writerData[MAX_WRITERS_L1];

    // Synchronization state
    int readerCount;
    int writerCount;
} SharedDataL1;

// Level 2 shared data structure (written by aggregator, read by 3 readers)
typedef struct
{
    int activeReaders;
    int waitingReaders;
    int aggregatedMessageCount;

    // Aggregated data from Level 1
    char aggregatedData[MAX_AGGREGATED_SIZE];
    int totalMessages;
    time_t lastUpdateTime;
    double averageTimestamp;

    // Statistics
    int messagesFromWriter[MAX_WRITERS_L1];

    // Synchronization state
    int readerCount;
    int writerCount;
} SharedDataL2;

// Aggregator control structure
typedef struct
{
    int isRunning;
    int processedCount;
    time_t startTime;
    int aggregationInterval; // milliseconds
} AggregatorControl;

// Legacy compatibility - maintain original structure for existing code
typedef struct
{
    int readerCount;
    int writerCount;
    int waitingWriters;
    int waitingReaders;
    int isPriorityWriter;
    char message[256];
    int messageId;
} SharedData;

// Size definitions
#define SHARED_MEM_L1_SIZE sizeof(SharedDataL1)
#define SHARED_MEM_L2_SIZE sizeof(SharedDataL2)
#define SHARED_MEM_SIZE sizeof(SharedData) // Legacy compatibility

// Process types for the 3-level system
typedef enum
{
    PROCESS_WRITER_L1,
    PROCESS_AGGREGATOR_L2,
    PROCESS_READER_L3
} ProcessType;

#endif // COMMON_H