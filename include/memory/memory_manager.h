#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

// Memory allocation strategies
typedef enum
{
    SEGMENTATION,
    PAGING,
    HYBRID // A combination of both
} MemoryStrategy;

// Memory block structure for segmentation
typedef struct MemorySegment
{
    int id;                     // Segment ID
    size_t size;                // Size of the segment
    size_t address;             // Starting address
    bool allocated;             // Whether this segment is allocated
    char processName[32];       // Name of the process that owns this segment
    int processId;              // ID of the process that owns this segment
    char segmentType[16];       // Type: "code", "data", "stack", etc.
    struct MemorySegment *next; // Linked list implementation
} MemorySegment;

// Page structure for paging
typedef struct
{
    int id;             // Page ID
    size_t frameNumber; // Frame number in physical memory
    bool allocated;     // Whether this page is allocated
    int processId;      // Process ID that owns this page
    size_t usedBytes;   // How much of the page is actually used
} Page;

// Page table structure
typedef struct
{
    int processId;
    Page *pages;
    int pageCount;
} PageTable;

// Process structure
typedef struct
{
    int id;
    char name[32];
    size_t size;
    MemoryStrategy allocStrategy;

    // For segmentation
    MemorySegment *segments;
    int segmentCount;

    // For paging
    PageTable *pageTable;
} Process;

// Memory manager context
typedef struct
{
    // Common properties
    MemoryStrategy strategy;
    size_t totalMemory;
    size_t freeMemory;
    size_t usedMemory;

    // For segmentation
    MemorySegment *segmentList;

    // For paging
    size_t pageSize;
    size_t totalPages;
    size_t freePages;
    Page *pageFrames;

    // Process management
    Process *processes;
    int processCount;
    int maxProcesses;

    // Statistics
    double externalFragmentation;
    double internalFragmentation;
} MemoryManager;

// Memory manager initialization and cleanup
MemoryManager *createMemoryManager(MemoryStrategy strategy, size_t totalMemory, size_t pageSize, int maxProcesses);
void destroyMemoryManager(MemoryManager *manager);

// Process management
int createProcess(MemoryManager *manager, const char *name, size_t size);
bool terminateProcess(MemoryManager *manager, int processId);

// Segmentation functions
bool allocateSegment(MemoryManager *manager, int processId, const char *segmentType, size_t size);
void deallocateSegments(MemoryManager *manager, int processId);

// Paging functions
bool allocatePages(MemoryManager *manager, int processId, size_t size);
void deallocatePages(MemoryManager *manager, int processId);

// Memory statistics
void calculateFragmentation(MemoryManager *manager);
void printMemoryStats(MemoryManager *manager);
void visualizeMemory(MemoryManager *manager);
void visualizeMemoryGraphically(MemoryManager *manager);

#endif // MEMORY_MANAGER_H
