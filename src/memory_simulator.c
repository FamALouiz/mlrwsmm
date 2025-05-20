#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/memory/memory_manager.h"
#include "../include/log/logger.h"

#define DEFAULT_TOTAL_MEMORY 1048576 // 1MB
#define DEFAULT_PAGE_SIZE 4096       // 4KB
#define MAX_PROCESSES 100

void displayMenu();
void runSegmentationDemo(size_t totalMemory);
void runPagingDemo(size_t totalMemory, size_t pageSize);
void runHybridDemo(size_t totalMemory, size_t pageSize);

int main(int argc, char *argv[])
{
    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    info("Memory Management Simulator");
    info("==========================");
    info("This simulator demonstrates memory allocation using segmentation and paging");

    size_t totalMemory = DEFAULT_TOTAL_MEMORY;
    size_t pageSize = DEFAULT_PAGE_SIZE;

    // Parse command line arguments if provided
    if (argc > 1)
    {
        totalMemory = atoi(argv[1]);
        if (totalMemory <= 0)
        {
            totalMemory = DEFAULT_TOTAL_MEMORY;
        }
    }

    if (argc > 2)
    {
        pageSize = atoi(argv[2]);
        if (pageSize <= 0 || pageSize > totalMemory / 10)
        {
            pageSize = DEFAULT_PAGE_SIZE;
        }
    }

    char configMsg[100];
    sprintf(configMsg, "Total Memory: %zu bytes, Page Size: %zu bytes", totalMemory, pageSize);
    info(configMsg);

    char choice;
    bool running = 1;

    while (running)
    {
        displayMenu();
        info("Enter choice: ");
        scanf(" %c", &choice);

        switch (choice)
        {
        case '1':
            runSegmentationDemo(totalMemory);
            break;

        case '2':
            runPagingDemo(totalMemory, pageSize);
            break;

        case '3':
            runHybridDemo(totalMemory, pageSize);
            break;

        case '4':
            info("Exiting Memory Management Simulator...");
            running = false;
            break;

        default:
            warn("Invalid choice. Please try again.");
        }
    }

    close_logger();
    return 0;
}

void displayMenu()
{
    info("\n------ Memory Management Menu ------");
    info("1. Segmentation Demonstration");
    info("2. Paging Demonstration");
    info("3. Hybrid (Segmentation + Paging) Demonstration");
    info("4. Exit");
}

// Run a demonstration using segmentation memory allocation
void runSegmentationDemo(size_t totalMemory)
{
    info("\n=== Starting Segmentation Demonstration ===");

    MemoryManager *manager = createMemoryManager(SEGMENTATION, totalMemory, 0, MAX_PROCESSES);
    if (!manager)
    {
        error("Failed to create memory manager");
        return;
    }

    // Initial memory state
    info("\nInitial memory state:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    // Demo process creation with different segment sizes
    info("Creating processes with different segment sizes...");

    // Create processes with varying sizes to demonstrate fragmentation
    int processIds[10];
    processIds[0] = createProcess(manager, "Process1", totalMemory / 10);
    info("After creating Process1 (10%% of memory):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    processIds[1] = createProcess(manager, "Process2", totalMemory / 5);
    info("After creating Process2 (20%% of memory):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    processIds[2] = createProcess(manager, "Process3", totalMemory / 8);
    info("After creating Process3 (12.5%% of memory):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    // Calculate and display fragmentation
    calculateFragmentation(manager);

    // Terminate a process in the middle to create fragmentation
    info("Terminating Process2 to create fragmentation...");
    terminateProcess(manager, processIds[1]);

    info("After terminating Process2:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);
    calculateFragmentation(manager);

    // Try to allocate a process larger than the largest free segment
    info("\nAttempting to create a process larger than the largest free segment...");
    processIds[3] = createProcess(manager, "Process4", totalMemory / 4);
    if (processIds[3] == -1)
    {
        info("Failed to create Process4 - demonstrates external fragmentation issue");
    }

    // Create a smaller process that should fit in the fragmented space
    info("\nCreating a smaller process that fits in the fragmented space...");
    processIds[4] = createProcess(manager, "Process5", totalMemory / 10);
    info("\nAfter creating Process5 (10%% of memory):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);
    calculateFragmentation(manager);

    // Cleanup
    info("\nCleaning up all processes...");
    for (int i = 0; i < 5; i++)
    {
        if (processIds[i] >= 0)
        {
            terminateProcess(manager, processIds[i]);
        }
    }

    info("\nFinal memory state:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    destroyMemoryManager(manager);
    info("=== Segmentation Demonstration Completed ===\n");
}

// Run a demonstration using paging memory allocation
void runPagingDemo(size_t totalMemory, size_t pageSize)
{
    info("\n=== Starting Paging Demonstration ===");

    MemoryManager *manager = createMemoryManager(PAGING, totalMemory, pageSize, MAX_PROCESSES);
    if (!manager)
    {
        error("Failed to create memory manager");
        return;
    }

    // Initial memory state
    info("\nInitial memory state:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    // Demo process creation with different sizes
    info("Creating processes with different sizes to demonstrate internal fragmentation...");

    // Create processes with sizes that don't align perfectly with page size
    int processIds[10];
    processIds[0] = createProcess(manager, "Process1", pageSize * 3 + 100); // Wastes almost a full page
    info("\nAfter creating Process1 (uses 3 pages plus 100 bytes):");
    info("Note: This will waste almost a full page - demonstrating internal fragmentation");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    processIds[1] = createProcess(manager, "Process2", pageSize * 2 - 200); // Slightly less than 2 pages
    info("\nAfter creating Process2 (uses slightly less than 2 pages):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    processIds[2] = createProcess(manager, "Process3", pageSize / 2); // Uses half a page
    info("\nAfter creating Process3 (uses only half a page):");
    info("Note: This wastes half a page - internal fragmentation");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    // Calculate internal fragmentation
    calculateFragmentation(manager);

    // Terminate some processes
    info("\nTerminating Process2 (freeing its pages)...");
    terminateProcess(manager, processIds[1]);

    info("\nAfter terminating Process2:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    // Create more processes
    info("\nCreating Process4 (requiring just over 4 pages)...");
    processIds[3] = createProcess(manager, "Process4", pageSize * 4 + 10); // Just over 4 pages

    info("\nAfter creating Process4:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);
    calculateFragmentation(manager);

    // Cleanup
    info("\nCleaning up all processes...");
    for (int i = 0; i < 4; i++)
    {
        if (processIds[i] >= 0)
        {
            terminateProcess(manager, processIds[i]);
        }
    }

    printMemoryStats(manager);
    visualizeMemory(manager);

    destroyMemoryManager(manager);
    info("=== Paging Demonstration Completed ===\n");
}

// Run a demonstration using hybrid memory allocation (both segmentation and paging)
void runHybridDemo(size_t totalMemory, size_t pageSize)
{
    info("\n=== Starting Hybrid (Segmentation + Paging) Demonstration ===");

    MemoryManager *manager = createMemoryManager(HYBRID, totalMemory, pageSize, MAX_PROCESSES);
    if (!manager)
    {
        error("Failed to create memory manager");
        return;
    }

    // Initial memory state
    info("\nInitial memory state:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    // Demo process creation with different sizes
    info("Creating processes with different sizes...");
    info("In hybrid mode, small allocations use paging, large ones use segmentation");

    int processIds[10];
    processIds[0] = createProcess(manager, "SmallProc1", pageSize); // 1 page - uses paging
    info("\nAfter creating SmallProc1 (exactly 1 page - uses paging):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    processIds[1] = createProcess(manager, "LargeProc1", pageSize * 10); // Large - uses segmentation
    info("\nAfter creating LargeProc1 (10 pages worth - uses segmentation):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    processIds[2] = createProcess(manager, "SmallProc2", pageSize * 2 + 100); // 2+ pages - uses paging
    info("\nAfter creating SmallProc2 (just over 2 pages - uses paging):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    processIds[3] = createProcess(manager, "MediumProc", pageSize * 5 - 200); // 5- pages - depends on threshold
    info("\nAfter creating MediumProc (just under 5 pages):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    // Calculate both types of fragmentation
    calculateFragmentation(manager);

    // Terminate some processes to create fragmentation
    info("\nTerminating processes to create both types of fragmentation...");
    info("Terminating LargeProc1 (frees segmentation space)");
    terminateProcess(manager, processIds[1]);
    info("\nAfter terminating LargeProc1:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    info("\nTerminating SmallProc2 (frees paged space)");
    terminateProcess(manager, processIds[2]);
    info("\nAfter terminating SmallProc2:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);
    calculateFragmentation(manager);

    // Create more processes with various sizes
    info("\nCreating more processes with various sizes...");
    processIds[4] = createProcess(manager, "LargeProc2", pageSize * 8);
    info("\nAfter creating LargeProc2 (8 pages worth - uses segmentation):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    processIds[5] = createProcess(manager, "TinyProc", pageSize / 4);
    info("\nAfter creating TinyProc (1/4 of a page - shows internal fragmentation):");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);
    calculateFragmentation(manager);

    // Cleanup
    info("\nCleaning up all processes...");
    for (int i = 0; i < 6; i++)
    {
        if (processIds[i] >= 0)
        {
            terminateProcess(manager, processIds[i]);
        }
    }

    info("\nFinal memory state after cleanup:");
    printMemoryStats(manager);
    visualizeMemory(manager);
    visualizeMemoryGraphically(manager);

    destroyMemoryManager(manager);
    info("=== Hybrid Demonstration Completed ===\n");
}
