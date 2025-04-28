#ifndef COMMON_H
#define COMMON_H

// Shared memory and synchronization definitions
#define SHARED_MEMORY_NAME "RWSharedMemory"
#define MUTEX_NAME "RWMutex"
#define WRITER_SEMAPHORE_NAME "WriterSemaphore"
#define READER_SEMAPHORE_NAME "ReaderSemaphore"
#define PRIORITY_MUTEX_NAME "PriorityMutex"

// Shared memory structure
typedef struct
{
    int readerCount;      // Number of active readers
    int writerCount;      // Number of active writers
    int waitingWriters;   // Number of writers waiting
    int waitingReaders;   // Number of readers waiting
    int isPriorityWriter; // Flag: 1 = writer priority, 0 = reader priority
    char message[256];    // The actual data being shared
    int messageId;        // A counter to help track message updates
} SharedData;

// Size of shared memory
#define SHARED_MEM_SIZE sizeof(SharedData)

#endif // COMMON_H