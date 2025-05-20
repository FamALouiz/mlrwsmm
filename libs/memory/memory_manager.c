#include "../../include/memory/memory_manager.h"
#include "../../include/log/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Create a new memory manager instance
MemoryManager *createMemoryManager(MemoryStrategy strategy, size_t totalMemory, size_t pageSize, int maxProcesses)
{
    MemoryManager *manager = (MemoryManager *)malloc(sizeof(MemoryManager));
    if (!manager)
    {
        error("Failed to allocate memory for memory manager");
        return NULL;
    }

    manager->strategy = strategy;
    manager->totalMemory = totalMemory;
    manager->freeMemory = totalMemory;
    manager->usedMemory = 0;
    manager->pageSize = pageSize;
    manager->externalFragmentation = 0.0;
    manager->internalFragmentation = 0.0;

    manager->processes = (Process *)malloc(maxProcesses * sizeof(Process));
    manager->processCount = 0;
    manager->maxProcesses = maxProcesses;

    // Initialize structures based on strategy
    switch (strategy)
    {
    case SEGMENTATION:
        // Initialize a single free segment that spans the entire memory
        manager->segmentList = (MemorySegment *)malloc(sizeof(MemorySegment));
        manager->segmentList->id = 0;
        manager->segmentList->size = totalMemory;
        manager->segmentList->address = 0;
        manager->segmentList->allocated = false;
        strcpy(manager->segmentList->processName, "none");
        manager->segmentList->processId = -1;
        strcpy(manager->segmentList->segmentType, "free");
        manager->segmentList->next = NULL;

        // Paging structures not needed
        manager->totalPages = 0;
        manager->freePages = 0;
        manager->pageFrames = NULL;
        break;

    case PAGING:
        // Calculate number of pages
        manager->totalPages = totalMemory / pageSize;
        manager->freePages = manager->totalPages;

        // Allocate page frames
        manager->pageFrames = (Page *)malloc(manager->totalPages * sizeof(Page));
        for (size_t i = 0; i < manager->totalPages; i++)
        {
            manager->pageFrames[i].id = i;
            manager->pageFrames[i].frameNumber = i;
            manager->pageFrames[i].allocated = false;
            manager->pageFrames[i].processId = -1;
            manager->pageFrames[i].usedBytes = 0;
        }

        // Segmentation structure set to NULL
        manager->segmentList = NULL;
        break;

    case HYBRID:
        // Initialize both segmentation and paging structures
        manager->segmentList = (MemorySegment *)malloc(sizeof(MemorySegment));
        manager->segmentList->id = 0;
        manager->segmentList->size = totalMemory;
        manager->segmentList->address = 0;
        manager->segmentList->allocated = false;
        strcpy(manager->segmentList->processName, "none");
        manager->segmentList->processId = -1;
        strcpy(manager->segmentList->segmentType, "free");
        manager->segmentList->next = NULL;

        // Also initialize paging structures
        manager->totalPages = totalMemory / pageSize;
        manager->freePages = manager->totalPages;

        manager->pageFrames = (Page *)malloc(manager->totalPages * sizeof(Page));
        for (size_t i = 0; i < manager->totalPages; i++)
        {
            manager->pageFrames[i].id = i;
            manager->pageFrames[i].frameNumber = i;
            manager->pageFrames[i].allocated = false;
            manager->pageFrames[i].processId = -1;
            manager->pageFrames[i].usedBytes = 0;
        }
        break;
    }

    char logMsg[100];
    sprintf(logMsg, "Memory manager created with strategy: %s, total memory: %zu bytes",
            strategy == SEGMENTATION ? "Segmentation" : strategy == PAGING ? "Paging"
                                                                           : "Hybrid",
            totalMemory);
    info(logMsg);

    return manager;
}

// Clean up memory manager
void destroyMemoryManager(MemoryManager *manager)
{
    if (!manager)
        return;

    // Free segmentation structures
    MemorySegment *current = manager->segmentList;
    while (current)
    {
        MemorySegment *next = current->next;
        free(current);
        current = next;
    }

    // Free paging structures
    if (manager->pageFrames)
    {
        free(manager->pageFrames);
    }

    // Free processes and their structures
    for (int i = 0; i < manager->processCount; i++)
    {
        Process *proc = &manager->processes[i];

        // Free process segments
        MemorySegment *seg = proc->segments;
        while (seg)
        {
            MemorySegment *next = seg->next;
            free(seg);
            seg = next;
        }

        // Free page table
        if (proc->pageTable)
        {
            if (proc->pageTable->pages)
            {
                free(proc->pageTable->pages);
            }
            free(proc->pageTable);
        }
    }

    // Free processes array
    free(manager->processes);

    // Free the manager itself
    free(manager);

    info("Memory manager destroyed");
}

// Find a suitable segment using first-fit algorithm
MemorySegment *findFreeSegment(MemoryManager *manager, size_t size)
{
    MemorySegment *current = manager->segmentList;

    while (current)
    {
        if (!current->allocated && current->size >= size)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

// Find contiguous free pages
int findFreePages(MemoryManager *manager, size_t numPages)
{
    int consecutiveCount = 0;
    int startFrame = -1;

    for (size_t i = 0; i < manager->totalPages; i++)
    {
        if (!manager->pageFrames[i].allocated)
        {
            if (consecutiveCount == 0)
            {
                startFrame = i;
            }
            consecutiveCount++;

            if (consecutiveCount == numPages)
            {
                return startFrame;
            }
        }
        else
        {
            consecutiveCount = 0;
            startFrame = -1;
        }
    }

    return -1; // Not enough contiguous pages
}

// Create a new process
int createProcess(MemoryManager *manager, const char *name, size_t size)
{
    if (manager->processCount >= manager->maxProcesses)
    {
        error("Cannot create more processes: maximum limit reached");
        return -1;
    }

    int processId = manager->processCount;
    Process *proc = &manager->processes[processId];

    proc->id = processId;
    strncpy(proc->name, name, 31);
    proc->name[31] = '\0';
    proc->size = size;
    proc->allocStrategy = manager->strategy;
    proc->segments = NULL;
    proc->segmentCount = 0;
    proc->pageTable = NULL;

    bool success = false;

    switch (manager->strategy)
    {
    case SEGMENTATION:
        success = allocateSegment(manager, processId, "process", size);
        break;

    case PAGING:
        success = allocatePages(manager, processId, size);
        break;

    case HYBRID:
        // In hybrid mode, we'll use segmentation for large allocations and paging for small ones
        if (size > manager->pageSize * 4)
        {
            success = allocateSegment(manager, processId, "process", size);
        }
        else
        {
            success = allocatePages(manager, processId, size);
        }
        break;
    }

    if (success)
    {
        char logMsg[100];
        sprintf(logMsg, "Created process %s (ID: %d) with size: %zu bytes", name, processId, size);
        info(logMsg);
        manager->processCount++;
        return processId;
    }
    else
    {
        char logMsg[100];
        sprintf(logMsg, "Failed to create process %s: not enough memory", name);
        error(logMsg);
        return -1;
    }
}

// Terminate a process and free its memory
bool terminateProcess(MemoryManager *manager, int processId)
{
    if (processId < 0 || processId >= manager->processCount)
    {
        char errMsg[100];
        sprintf(errMsg, "Invalid process ID when terminating: %d", processId);
        error(errMsg);
        return false;
    }

    Process *proc = &manager->processes[processId];

    switch (manager->strategy)
    {
    case SEGMENTATION:
        deallocateSegments(manager, processId);
        break;

    case PAGING:
        deallocatePages(manager, processId);
        break;

    case HYBRID:
        // Free both segment and page allocations
        deallocateSegments(manager, processId);
        deallocatePages(manager, processId);
        break;
    }

    char logMsg[100];
    sprintf(logMsg, "Terminated process %s (ID: %d)", proc->name, processId);
    info(logMsg);

    return true;
}

// Split a segment for allocation
void splitSegment(MemoryManager *manager, MemorySegment *segment, size_t size, int processId, const char *segmentType)
{
    if (segment->size > size)
    {
        // Create a new segment for the remainder
        MemorySegment *newSegment = (MemorySegment *)malloc(sizeof(MemorySegment));
        newSegment->id = segment->id + 1;
        newSegment->size = segment->size - size;
        newSegment->address = segment->address + size;
        newSegment->allocated = false;
        strcpy(newSegment->processName, "none");
        newSegment->processId = -1;
        strcpy(newSegment->segmentType, "free");

        // Insert new segment into list
        newSegment->next = segment->next;
        segment->next = newSegment;
    }

    // Update the original segment
    segment->size = size;
    segment->allocated = true;
    strncpy(segment->processName, manager->processes[processId].name, 31);
    segment->processName[31] = '\0';
    segment->processId = processId;
    strncpy(segment->segmentType, segmentType, 15);
    segment->segmentType[15] = '\0';

    // Update memory stats
    manager->usedMemory += size;
    manager->freeMemory -= size;
}

// Allocate a memory segment for a process
bool allocateSegment(MemoryManager *manager, int processId, const char *segmentType, size_t size)
{
    if (processId < 0 || processId > manager->processCount)
    {
        char errMsg[100];
        sprintf(errMsg, "Invalid process ID when allocating: %d", processId);
        error(errMsg);
        return false;
    }

    // Find a suitable free segment
    MemorySegment *segment = findFreeSegment(manager, size);
    if (!segment)
    {
        char errMsg[100];
        sprintf(errMsg, "Failed to allocate segment: no suitable free segment of size %zu found", size);
        error(errMsg);
        return false;
    }

    // Split and allocate the segment
    splitSegment(manager, segment, size, processId, segmentType);

    // Add segment to process
    Process *proc = &manager->processes[processId];
    proc->segmentCount++;

    // Create a copy of the segment for the process
    MemorySegment *procSegment = (MemorySegment *)malloc(sizeof(MemorySegment));
    memcpy(procSegment, segment, sizeof(MemorySegment));
    procSegment->next = proc->segments;
    proc->segments = procSegment;

    char logMsg[100];
    sprintf(logMsg, "Allocated %s segment of size %zu for process %s at address %zu",
            segmentType, size, proc->name, segment->address);
    info(logMsg);

    return true;
}

// Merge adjacent free segments
void mergeAdjacentFreeSegments(MemoryManager *manager)
{
    MemorySegment *current = manager->segmentList;

    while (current && current->next)
    {
        if (!current->allocated && !current->next->allocated)
        {
            // Merge with next segment
            MemorySegment *toDelete = current->next;
            current->size += toDelete->size;
            current->next = toDelete->next;
            free(toDelete);
        }
        else
        {
            current = current->next;
        }
    }
}

// Free all segments allocated to a process
void deallocateSegments(MemoryManager *manager, int processId)
{
    if (processId < 0 || processId > manager->processCount)
    {
        return;
    }

    // Mark all segments belonging to this process as free
    MemorySegment *current = manager->segmentList;
    size_t freedMemory = 0;

    while (current)
    {
        if (current->allocated && current->processId == processId)
        {
            current->allocated = false;
            strcpy(current->processName, "none");
            current->processId = -1;
            strcpy(current->segmentType, "free");
            freedMemory += current->size;
        }
        current = current->next;
    }

    // Update memory stats
    manager->usedMemory -= freedMemory;
    manager->freeMemory += freedMemory;

    // Free process segments list
    Process *proc = &manager->processes[processId];
    MemorySegment *seg = proc->segments;
    while (seg)
    {
        MemorySegment *next = seg->next;
        free(seg);
        seg = next;
    }
    proc->segments = NULL;
    proc->segmentCount = 0;

    // Merge adjacent free segments
    mergeAdjacentFreeSegments(manager);

    char logMsg[100];
    sprintf(logMsg, "Deallocated all segments for process %s (ID: %d), freed %zu bytes",
            proc->name, processId, freedMemory);
    info(logMsg);
}

// Allocate pages for a process
bool allocatePages(MemoryManager *manager, int processId, size_t size)
{
    if (processId < 0 || processId > manager->processCount)
    {
        char errMsg[100];
        sprintf(errMsg, "Invalid process ID when allocating: %d", processId);
        error(errMsg);
        return false;
    }

    Process *proc = &manager->processes[processId];

    // Calculate how many pages are needed
    size_t numPages = (size + manager->pageSize - 1) / manager->pageSize;

    if (numPages > manager->freePages)
    {
        char errMsg[100];
        sprintf(errMsg, "Not enough free pages. Required: %zu, Available: %zu", numPages, manager->freePages);
        error(errMsg);
        return false;
    }

    // Allocate page table for the process
    proc->pageTable = (PageTable *)malloc(sizeof(PageTable));
    proc->pageTable->processId = processId;
    proc->pageTable->pageCount = numPages;
    proc->pageTable->pages = (Page *)malloc(numPages * sizeof(Page));

    // Assign pages using either contiguous or scattered allocation based on availability
    int startFrame = findFreePages(manager, numPages);

    if (startFrame >= 0)
    {
        // Contiguous allocation possible
        for (size_t i = 0; i < numPages; i++)
        {
            size_t frameIndex = startFrame + i;
            Page *frame = &manager->pageFrames[frameIndex];
            frame->allocated = true;
            frame->processId = processId;

            // Calculate how much of this page is actually used
            size_t usedInPage = (i == numPages - 1) ? (size % manager->pageSize == 0 ? manager->pageSize : size % manager->pageSize) : manager->pageSize;

            frame->usedBytes = usedInPage;

            // Copy page info to process page table
            memcpy(&proc->pageTable->pages[i], frame, sizeof(Page));
        }
    }
    else
    {
        // Need to use scattered allocation
        size_t pageCount = 0;

        for (size_t i = 0; i < manager->totalPages && pageCount < numPages; i++)
        {
            if (!manager->pageFrames[i].allocated)
            {
                Page *frame = &manager->pageFrames[i];
                frame->allocated = true;
                frame->processId = processId;

                // Calculate how much of this page is actually used
                size_t usedInPage = (pageCount == numPages - 1) ? (size % manager->pageSize == 0 ? manager->pageSize : size % manager->pageSize) : manager->pageSize;

                frame->usedBytes = usedInPage;

                // Copy page info to process page table
                memcpy(&proc->pageTable->pages[pageCount], frame, sizeof(Page));

                pageCount++;
            }
        }
    }

    // Update memory stats
    manager->freePages -= numPages;
    manager->usedMemory += size;
    manager->freeMemory -= numPages * manager->pageSize;

    char logMsg[100];
    sprintf(logMsg, "Allocated %zu pages (%zu bytes) for process %s (ID: %d)",
            numPages, numPages * manager->pageSize, proc->name, processId);
    info(logMsg);

    return true;
}

// Free all pages allocated to a process
void deallocatePages(MemoryManager *manager, int processId)
{
    if (processId < 0 || processId > manager->processCount)
    {
        return;
    }

    Process *proc = &manager->processes[processId];

    if (!proc->pageTable)
    {
        return; // No pages allocated
    }

    size_t numPages = proc->pageTable->pageCount;
    size_t freedMemory = 0;

    // Free all frames allocated to this process
    for (size_t i = 0; i < manager->totalPages; i++)
    {
        if (manager->pageFrames[i].allocated && manager->pageFrames[i].processId == processId)
        {
            freedMemory += manager->pageFrames[i].usedBytes;
            manager->pageFrames[i].allocated = false;
            manager->pageFrames[i].processId = -1;
            manager->pageFrames[i].usedBytes = 0;
        }
    }

    // Free the page table
    free(proc->pageTable->pages);
    free(proc->pageTable);
    proc->pageTable = NULL;

    // Update memory stats
    manager->freePages += numPages;
    manager->usedMemory -= freedMemory;
    manager->freeMemory += numPages * manager->pageSize;

    char logMsg[100];
    sprintf(logMsg, "Deallocated %zu pages for process %s (ID: %d), freed %zu bytes",
            numPages, proc->name, processId, freedMemory);
    info(logMsg);
}

// Calculate memory fragmentation statistics
void calculateFragmentation(MemoryManager *manager)
{
    if (!manager)
        return;

    char header[100];
    sprintf(header, "======== Fragmentation Analysis ========");
    info(header);

    switch (manager->strategy)
    {
    case SEGMENTATION:
    {
        // Calculate external fragmentation
        size_t totalFreeMemory = 0;
        size_t largestFreeBlock = 0;
        int freeBlockCount = 0;

        MemorySegment *current = manager->segmentList;
        while (current)
        {
            if (!current->allocated)
            {
                totalFreeMemory += current->size;
                freeBlockCount++;
                if (current->size > largestFreeBlock)
                {
                    largestFreeBlock = current->size;
                }
            }
            current = current->next;
        }

        // External fragmentation is measured as 1 - (largest free block / total free memory)
        if (totalFreeMemory > 0)
        {
            manager->externalFragmentation = 1.0 - ((double)largestFreeBlock / totalFreeMemory);
        }
        else
        {
            manager->externalFragmentation = 0.0;
        }

        char logMsg[150];
        sprintf(logMsg, "Segmentation: External Fragmentation: %.2f%% (Free blocks: %d, Total free: %zu, Largest free: %zu)",
                manager->externalFragmentation * 100, freeBlockCount, totalFreeMemory, largestFreeBlock);
        info(logMsg);
        break;
    }

    case PAGING:
    {
        // Calculate internal fragmentation
        size_t wastedSpace = 0;
        size_t totalAllocated = 0;

        for (size_t i = 0; i < manager->totalPages; i++)
        {
            if (manager->pageFrames[i].allocated)
            {
                wastedSpace += (manager->pageSize - manager->pageFrames[i].usedBytes);
                totalAllocated += manager->pageSize;
            }
        }

        // Internal fragmentation is measured as wasted space / total allocated space
        if (totalAllocated > 0)
        {
            manager->internalFragmentation = (double)wastedSpace / totalAllocated;
        }
        else
        {
            manager->internalFragmentation = 0.0;
        }

        char logMsg[150];
        sprintf(logMsg, "Paging: Internal Fragmentation: %.2f%% (Wasted: %zu bytes out of %zu allocated)",
                manager->internalFragmentation * 100, wastedSpace, totalAllocated);
        info(logMsg);
        break;
    }

    case HYBRID:
    {
        // Calculate both types of fragmentation

        // External fragmentation (segmentation)
        size_t totalFreeMemory = 0;
        size_t largestFreeBlock = 0;
        int freeBlockCount = 0;

        MemorySegment *current = manager->segmentList;
        while (current)
        {
            if (!current->allocated)
            {
                totalFreeMemory += current->size;
                freeBlockCount++;
                if (current->size > largestFreeBlock)
                {
                    largestFreeBlock = current->size;
                }
            }
            current = current->next;
        }

        if (totalFreeMemory > 0)
        {
            manager->externalFragmentation = 1.0 - ((double)largestFreeBlock / totalFreeMemory);
        }
        else
        {
            manager->externalFragmentation = 0.0;
        }

        // Internal fragmentation (paging)
        size_t wastedSpace = 0;
        size_t totalAllocated = 0;

        for (size_t i = 0; i < manager->totalPages; i++)
        {
            if (manager->pageFrames[i].allocated)
            {
                wastedSpace += (manager->pageSize - manager->pageFrames[i].usedBytes);
                totalAllocated += manager->pageSize;
            }
        }

        if (totalAllocated > 0)
        {
            manager->internalFragmentation = (double)wastedSpace / totalAllocated;
        }
        else
        {
            manager->internalFragmentation = 0.0;
        }

        char logMsg[200];
        sprintf(logMsg, "Hybrid: External Fragmentation: %.2f%%, Internal Fragmentation: %.2f%%",
                manager->externalFragmentation * 100, manager->internalFragmentation * 100);
        info(logMsg);
        break;
    }
    }
}

// Print memory statistics
void printMemoryStats(MemoryManager *manager)
{
    if (!manager)
        return;

    char header[100];
    sprintf(header, "======== Memory Manager Statistics ========");
    info(header);

    char strategyMsg[100];
    sprintf(strategyMsg, "Strategy: %s",
            manager->strategy == SEGMENTATION ? "Segmentation" : manager->strategy == PAGING ? "Paging"
                                                                                             : "Hybrid");
    info(strategyMsg);

    char memStats[150];
    sprintf(memStats, "Total memory: %zu bytes | Used: %zu bytes (%.1f%%) | Free: %zu bytes (%.1f%%)",
            manager->totalMemory,
            manager->usedMemory,
            ((double)manager->usedMemory / manager->totalMemory) * 100,
            manager->freeMemory,
            ((double)manager->freeMemory / manager->totalMemory) * 100);
    info(memStats);

    // Calculate updated fragmentation statistics
    calculateFragmentation(manager);

    char procHeader[100];
    sprintf(procHeader, "---- Active Processes: %d ----", manager->processCount);
    info(procHeader);

    for (int i = 0; i < manager->processCount; i++)
    {
        Process *proc = &manager->processes[i];
        char procInfo[150];
        sprintf(procInfo, "Process %d: Name: %s, Size: %zu bytes",
                proc->id, proc->name, proc->size);
        info(procInfo);

        switch (manager->strategy)
        {
        case SEGMENTATION:
            // Print segment information
            info("  Segments:");
            MemorySegment *seg = proc->segments;
            while (seg)
            {
                char segInfo[150];
                sprintf(segInfo, "  - %s segment: Address: %zu, Size: %zu bytes",
                        seg->segmentType, seg->address, seg->size);
                info(segInfo);
                seg = seg->next;
            }
            break;

        case PAGING:
            // Print page table information
            if (proc->pageTable)
            {
                char pageInfo[150];
                sprintf(pageInfo, "  Pages: %d pages (%zu bytes per page)",
                        proc->pageTable->pageCount, manager->pageSize);
                info(pageInfo);
            }
            break;

        case HYBRID:
            // Print both segment and page information
            MemorySegment *hybridSeg = proc->segments;
            if (hybridSeg)
            {
                info("  Segments:");
                while (hybridSeg)
                {
                    char segInfo[150];
                    sprintf(segInfo, "  - %s segment: Address: %zu, Size: %zu bytes",
                            hybridSeg->segmentType, hybridSeg->address, hybridSeg->size);
                    info(segInfo);
                    hybridSeg = hybridSeg->next;
                }
            }

            if (proc->pageTable)
            {
                char pageInfo[150];
                sprintf(pageInfo, "  Pages: %d pages (%zu bytes per page)",
                        proc->pageTable->pageCount, manager->pageSize);
                info(pageInfo);
            }
            break;
        }
    }

    char footer[100];
    sprintf(footer, "==========================================");
    info(footer);
}

// Visualize the memory layout
void visualizeMemory(MemoryManager *manager)
{
    if (!manager)
        return;

    char header[100];
    sprintf(header, "======== Memory Visualization ========");
    info(header);

    switch (manager->strategy)
    {
    case SEGMENTATION:
    {
        info("Memory Layout (Segmentation):");
        info("| Address | Size     | Status    | Process   | Type     |");
        info("|---------|----------|-----------|-----------|----------|");

        MemorySegment *current = manager->segmentList;
        while (current)
        {
            char segmentInfo[100];
            sprintf(segmentInfo, "| %-7zu | %-8zu | %-9s | %-9s | %-8s |",
                    current->address,
                    current->size,
                    current->allocated ? "Allocated" : "Free",
                    current->allocated ? current->processName : "-",
                    current->allocated ? current->segmentType : "-");
            info(segmentInfo);

            current = current->next;
        }
        break;
    }

    case PAGING:
    {
        info("Memory Layout (Paging):");
        info("| Frame # | Status    | Process   | Used Bytes |");
        info("|---------|-----------|-----------|------------|");

        for (size_t i = 0; i < manager->totalPages; i++)
        {
            char frameInfo[100];
            sprintf(frameInfo, "| %-7zu | %-9s | %-9s | %-10zu |",
                    i,
                    manager->pageFrames[i].allocated ? "Allocated" : "Free",
                    manager->pageFrames[i].allocated ? manager->processes[manager->pageFrames[i].processId].name : "-",
                    manager->pageFrames[i].allocated ? manager->pageFrames[i].usedBytes : 0);
            info(frameInfo);
        }
        break;
    }

    case HYBRID:
    {
        info("Memory Layout (Hybrid - Segments):");
        info("| Address | Size     | Status    | Process   | Type     |");
        info("|---------|----------|-----------|-----------|----------|");

        MemorySegment *current = manager->segmentList;
        while (current)
        {
            char segmentInfo[100];
            sprintf(segmentInfo, "| %-7zu | %-8zu | %-9s | %-9s | %-8s |",
                    current->address,
                    current->size,
                    current->allocated ? "Allocated" : "Free",
                    current->allocated ? current->processName : "-",
                    current->allocated ? current->segmentType : "-");
            info(segmentInfo);

            current = current->next;
        }

        info("\nMemory Layout (Hybrid - Pages):");
        info("| Frame # | Status    | Process   | Used Bytes |");
        info("|---------|-----------|-----------|------------|");

        for (size_t i = 0; i < manager->totalPages; i++)
        {
            char frameInfo[100];
            sprintf(frameInfo, "| %-7zu | %-9s | %-9s | %-10zu |",
                    i,
                    manager->pageFrames[i].allocated ? "Allocated" : "Free",
                    manager->pageFrames[i].allocated ? manager->processes[manager->pageFrames[i].processId].name : "-",
                    manager->pageFrames[i].allocated ? manager->pageFrames[i].usedBytes : 0);
            info(frameInfo);
        }
        break;
    }
    }

    char footer[100];
    sprintf(footer, "=====================================");
    info(footer);
}

// Visualize the memory layout with a graphical representation
void visualizeMemoryGraphically(MemoryManager *manager)
{
    if (!manager)
        return;

    char header[100];
    sprintf(header, "======== Memory Visualization (Graphical) ========");
    info(header);

    // Define the display width for visualization
    const int displayWidth = 80;
    char visualLine[81];

    switch (manager->strategy)
    {
    case SEGMENTATION:
    {
        info("Memory Layout (Segmentation):");
        info("Each character represents a memory unit");
        info("Legend: # = Allocated, . = Free");

        // Count total memory units to scale properly
        size_t totalMemSize = manager->totalMemory;
        double scaleFactor = (double)displayWidth / totalMemSize;

        // Initialize the display
        for (int i = 0; i < displayWidth; i++)
            visualLine[i] = ' ';
        visualLine[displayWidth] = '\0';

        // Fill in segments
        MemorySegment *current = manager->segmentList;
        while (current)
        {
            int startPos = (int)(current->address * scaleFactor);
            int endPos = (int)((current->address + current->size) * scaleFactor);

            // Ensure we don't overflow the buffer
            if (startPos >= displayWidth)
                startPos = displayWidth - 1;
            endPos = (endPos >= displayWidth) ? displayWidth - 1 : endPos;

            char blockChar = current->allocated ? '#' : '.';

            for (int i = startPos; i <= endPos && i < displayWidth; i++)
                visualLine[i] = blockChar;

            current = current->next;
        }

        // Print memory visualization
        info(visualLine);

        // Show markers for addresses
        info("0                                                                              100%%");

        // Show process labels beneath the visualization
        current = manager->segmentList;
        while (current)
        {
            if (current->allocated)
            {
                // Calculate position for the label
                int labelPos = (int)(current->address * scaleFactor);
                int labelEndPos = (int)((current->address + current->size) * scaleFactor);
                int centerPos = (labelPos + labelEndPos) / 2;

                // Don't print if segment is too small
                if (labelEndPos - labelPos > 3)
                {
                    // Create a label line
                    for (int i = 0; i < displayWidth; i++)
                        visualLine[i] = ' ';

                    // Place the process name at the center position
                    int nameLen = strlen(current->processName);
                    int startNamePos = centerPos - nameLen / 2;

                    // Make sure we stay within bounds
                    if (startNamePos < 0)
                        startNamePos = 0;
                    if (startNamePos + nameLen >= displayWidth)
                        startNamePos = displayWidth - nameLen - 1;

                    // Insert the process name
                    for (int i = 0; i < nameLen && startNamePos + i < displayWidth; i++)
                        visualLine[startNamePos + i] = current->processName[i];

                    info(visualLine);
                }
            }
            current = current->next;
        }
        break;
    }

    case PAGING:
    {
        info("Memory Layout (Paging):");
        info("Each character represents a page frame");
        info("Legend: # = Allocated, . = Free");

        // Initialize the display
        int totalPages = manager->totalPages;
        int pagesPerRow = displayWidth;

        // If we have more pages than can fit in one row
        if (totalPages > displayWidth)
            pagesPerRow = displayWidth;

        int rowCount = (totalPages + pagesPerRow - 1) / pagesPerRow;

        for (int row = 0; row < rowCount; row++)
        {
            // Initialize the line
            for (int i = 0; i < displayWidth; i++)
                visualLine[i] = ' ';
            visualLine[displayWidth] = '\0';

            // Fill the line with page frames
            for (int i = 0; i < pagesPerRow && (row * pagesPerRow + i) < totalPages; i++)
            {
                size_t pageNum = row * pagesPerRow + i;
                visualLine[i] = manager->pageFrames[pageNum].allocated ? '#' : '.';
            }

            info(visualLine);
        }

        // Show process labels beneath the visualization
        info("Process Ownership:");

        for (int i = 0; i < manager->processCount; i++)
        {
            Process *proc = &manager->processes[i];
            if (proc->pageTable == NULL)
                continue;

            // Find all pages associated with this process
            for (int row = 0; row < rowCount; row++)
            {
                // Reset line
                for (int j = 0; j < displayWidth; j++)
                    visualLine[j] = ' ';

                bool hasPageInRow = false;

                // Check each page in this row
                for (int col = 0; col < pagesPerRow && (row * pagesPerRow + col) < totalPages; col++)
                {
                    size_t pageNum = row * pagesPerRow + col;

                    // Check if this page belongs to this process
                    if (manager->pageFrames[pageNum].allocated &&
                        manager->pageFrames[pageNum].processId == proc->id)
                    {
                        visualLine[col] = 'P';
                        hasPageInRow = true;
                    }
                }

                if (hasPageInRow)
                {
                    char label[100];
                    sprintf(label, " Process %s", proc->name);

                    // Append process name to the visualization
                    int nameLen = strlen(label);
                    for (int j = 0; j < nameLen && j + pagesPerRow + 1 < displayWidth; j++)
                        visualLine[pagesPerRow + 1 + j] = label[j];

                    info(visualLine);
                }
            }
        }
        break;
    }

    case HYBRID:
    {
        info("Memory Layout (Hybrid):");
        info("Segmentation area:");
        info("Each character represents a memory unit");
        info("Legend: # = Allocated, . = Free");

        // Visualize segmentation area
        size_t segMemSize = manager->totalMemory / 2; // Assume half for segmentation
        double scaleFactor = (double)displayWidth / segMemSize;

        // Initialize the display
        for (int i = 0; i < displayWidth; i++)
            visualLine[i] = ' ';
        visualLine[displayWidth] = '\0';

        // Fill in segments
        MemorySegment *current = manager->segmentList;
        while (current)
        {
            int startPos = (int)(current->address * scaleFactor);
            int endPos = (int)((current->address + current->size) * scaleFactor);

            // Ensure we don't overflow the buffer
            endPos = (endPos >= displayWidth) ? displayWidth - 1 : endPos;

            char blockChar = current->allocated ? '#' : '.';

            for (int i = startPos; i <= endPos && i < displayWidth; i++)
                visualLine[i] = blockChar;

            current = current->next;
        }

        // Print memory visualization for segmentation
        info(visualLine);

        // Visualize paging area
        info("\nPaging area:");
        info("Each character represents a page frame");
        info("Legend: # = Allocated, . = Free");

        // Initialize the display
        int totalPages = manager->totalPages;
        int pagesPerRow = displayWidth;

        int rowCount = (totalPages + pagesPerRow - 1) / pagesPerRow;

        for (int row = 0; row < rowCount; row++)
        {
            // Initialize the line
            for (int i = 0; i < displayWidth; i++)
                visualLine[i] = ' ';

            // Fill the line with page frames
            for (int i = 0; i < pagesPerRow && (row * pagesPerRow + i) < totalPages; i++)
            {
                size_t pageNum = row * pagesPerRow + i;
                visualLine[i] = manager->pageFrames[pageNum].allocated ? '#' : '.';
            }

            info(visualLine);
        }

        break;
    }
    }

    // Show fragmentation statistics
    char fragMsg[150];
    switch (manager->strategy)
    {
    case SEGMENTATION:
        sprintf(fragMsg, "External Fragmentation: %.2f%%", manager->externalFragmentation * 100);
        break;
    case PAGING:
        sprintf(fragMsg, "Internal Fragmentation: %.2f%%", manager->internalFragmentation * 100);
        break;
    case HYBRID:
        sprintf(fragMsg, "External Fragmentation: %.2f%%, Internal Fragmentation: %.2f%%",
                manager->externalFragmentation * 100,
                manager->internalFragmentation * 100);
        break;
    }
    info(fragMsg);

    char footer[100];
    sprintf(footer, "===============================================");
    info(footer);
}
