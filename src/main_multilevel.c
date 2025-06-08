#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/common.h"
#include "../include/log/logger.h"
#include "../include/platform/process.h"
#include "../include/platform/shared_memory.h"
#include "../include/platform/sync.h"
#include "../include/path/path.h"

#define MAX_COMMAND_LENGTH 256
#define MAX_PROCESSES 20

typedef struct
{
    ProcessHandle *handle;
    int id;
    ProcessType type;
    char description[100];
    bool isActive;
} ProcessInfo;

ProcessInfo processes[MAX_PROCESSES];
int processCount = 0;

// Function declarations
bool createL1Writer(int writerId);
bool createL2Aggregator();
bool createL3Reader(int readerId);
void displayMenu();
void cleanupProcesses();
void displayActiveProcesses();
void startCompleteSystem();
void displaySystemStatus();

int main()
{
    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    info("==================================================================");
    info("  Multi-Level Generalized Reader-Writer System - Milestone 3");
    info("==================================================================");
    info("System Architecture:");
    info("  Level 1: 3 Writers -> Shared Memory 1");
    info("  Level 2: 1 Aggregator (Reader L1 -> Writer L2) -> Shared Memory 2");
    info("  Level 3: 3 Readers <- Shared Memory 2");
    info("==================================================================");

    // Initialize process array
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        processes[i].isActive = false;
    }

    char choice;
    bool running = true;

    while (running)
    {
        displayMenu();
        info("Enter choice: ");
        scanf(" %c", &choice);

        switch (choice)
        {
        case '1':
            startCompleteSystem();
            break;

        case '2':
        {
            int id;
            info("Enter L1 Writer ID (1-3): ");
            scanf("%d", &id);
            if (id >= 1 && id <= MAX_WRITERS_L1)
            {
                createL1Writer(id);
            }
            else
            {
                warn("Invalid Writer ID. Must be between 1 and 3.");
            }
        }
        break;

        case '3':
            createL2Aggregator();
            break;

        case '4':
        {
            int id;
            info("Enter L3 Reader ID (1-3): ");
            scanf("%d", &id);
            if (id >= 1 && id <= MAX_READERS_L3)
            {
                createL3Reader(id);
            }
            else
            {
                warn("Invalid Reader ID. Must be between 1 and 3.");
            }
        }
        break;

        case '5':
            displayActiveProcesses();
            break;

        case '6':
            displaySystemStatus();
            break;

        case '7':
        {
            int index;
            info("Enter process index to terminate: ");
            scanf("%d", &index);

            if (index >= 0 && index < processCount && processes[index].isActive)
            {
                char terminateMsg[100];
                sprintf(terminateMsg, "Terminating %s...", processes[index].description);
                info(terminateMsg);

                terminate_process(processes[index].handle);
                close_process_handle(processes[index].handle);
                processes[index].isActive = false;

                info("Process terminated.");
            }
            else
            {
                warn("Invalid process index.");
            }
        }
        break;

        case 'q':
        case 'Q':
            info("Terminating all processes and exiting...");
            running = false;
            break;

        default:
            warn("Invalid choice. Please try again.");
        }
    }

    cleanupProcesses();
    info("All processes terminated. Multi-level system shutdown complete!");
    close_logger();
    return 0;
}

void startCompleteSystem()
{
    info("Starting complete 3-level system...");

    // Start Level 1 Writers (3 writers)
    for (int i = 1; i <= MAX_WRITERS_L1; i++)
    {
        char msg[100];
        sprintf(msg, "Starting L1 Writer %d...", i);
        info(msg);
        createL1Writer(i);
        platform_sleep(500); // Small delay between starts
    }

    // Start Level 2 Aggregator
    platform_sleep(2000); // Give writers time to initialize
    info("Starting L2 Aggregator...");
    createL2Aggregator();

    // Start Level 3 Readers (3 readers)
    platform_sleep(2000); // Give aggregator time to initialize
    for (int i = 1; i <= MAX_READERS_L3; i++)
    {
        char msg[100];
        sprintf(msg, "Starting L3 Reader %d...", i);
        info(msg);
        createL3Reader(i);
        platform_sleep(500); // Small delay between starts
    }

    info("Complete 3-level system started successfully!");
    info("Monitor the logs to see the data flow: L1 -> L2 -> L3");
}

bool createL1Writer(int writerId)
{
    char command[MAX_COMMAND_LENGTH];

#if defined(_WIN32)
    const char *pathParts[] = {"build", "bin", "writer_l1.exe"};
#else
    const char *pathParts[] = {"build", "bin", "writer_l1"};
#endif

    char exePath[MAX_COMMAND_LENGTH];
    join_paths(exePath, 3, pathParts);
    sprintf(command, "%s %d", exePath, writerId);

    info(command);

    ProcessHandle *handle = NULL;
    if (!create_process(command, writerId, 'W', &handle))
    {
        char errorMsg[100];
        sprintf(errorMsg, "Failed to create L1 Writer %d", writerId);
        error(errorMsg);
        return false;
    }

    processes[processCount].handle = handle;
    processes[processCount].id = writerId;
    processes[processCount].type = PROCESS_WRITER_L1;
    sprintf(processes[processCount].description, "L1 Writer %d", writerId);
    processes[processCount].isActive = true;
    processCount++;

    char successMsg[100];
    sprintf(successMsg, "L1 Writer %d started successfully (Process %d)", writerId, processCount - 1);
    info(successMsg);
    return true;
}

bool createL2Aggregator()
{
    char command[MAX_COMMAND_LENGTH];

#if defined(_WIN32)
    const char *pathParts[] = {"build", "bin", "aggregator_l2.exe"};
#else
    const char *pathParts[] = {"build", "bin", "aggregator_l2"};
#endif

    char exePath[MAX_COMMAND_LENGTH];
    join_paths(exePath, 3, pathParts);
    strcpy(command, exePath);

    ProcessHandle *handle = NULL;
    if (!create_process(command, 1, 'A', &handle))
    {
        error("Failed to create L2 Aggregator");
        return false;
    }

    processes[processCount].handle = handle;
    processes[processCount].id = 1;
    processes[processCount].type = PROCESS_AGGREGATOR_L2;
    sprintf(processes[processCount].description, "L2 Aggregator");
    processes[processCount].isActive = true;
    processCount++;

    char successMsg[100];
    sprintf(successMsg, "L2 Aggregator started successfully (Process %d)", processCount - 1);
    info(successMsg);
    return true;
}

bool createL3Reader(int readerId)
{
    char command[MAX_COMMAND_LENGTH];

#if defined(_WIN32)
    const char *pathParts[] = {"build", "bin", "reader_l3.exe"};
#else
    const char *pathParts[] = {"build", "bin", "reader_l3"};
#endif

    char exePath[MAX_COMMAND_LENGTH];
    join_paths(exePath, 3, pathParts);
    sprintf(command, "%s %d", exePath, readerId);

    ProcessHandle *handle = NULL;
    if (!create_process(command, readerId, 'R', &handle))
    {
        char errorMsg[100];
        sprintf(errorMsg, "Failed to create L3 Reader %d", readerId);
        error(errorMsg);
        return false;
    }

    processes[processCount].handle = handle;
    processes[processCount].id = readerId;
    processes[processCount].type = PROCESS_READER_L3;
    sprintf(processes[processCount].description, "L3 Reader %d", readerId);
    processes[processCount].isActive = true;
    processCount++;

    char successMsg[100];
    sprintf(successMsg, "L3 Reader %d started successfully (Process %d)", readerId, processCount - 1);
    info(successMsg);
    return true;
}

void displayActiveProcesses()
{
    info("\n=============== ACTIVE PROCESSES ===============");

    int activeCount = 0;
    for (int i = 0; i < processCount; i++)
    {
        if (processes[i].isActive)
        {
            char processInfo[150];
            sprintf(processInfo, "[%d] %s (PID: %lu)",
                    i, processes[i].description, get_process_id(processes[i].handle));
            info(processInfo);
            activeCount++;
        }
    }

    if (activeCount == 0)
    {
        info("No active processes.");
    }
    else
    {
        char countMsg[50];
        sprintf(countMsg, "Total active processes: %d", activeCount);
        info(countMsg);
    }
    info("===============================================\n");
}

void displaySystemStatus()
{
    info("\n=============== SYSTEM STATUS ===============");

    int l1Writers = 0, l2Aggregators = 0, l3Readers = 0;

    for (int i = 0; i < processCount; i++)
    {
        if (processes[i].isActive)
        {
            switch (processes[i].type)
            {
            case PROCESS_WRITER_L1:
                l1Writers++;
                break;
            case PROCESS_AGGREGATOR_L2:
                l2Aggregators++;
                break;
            case PROCESS_READER_L3:
                l3Readers++;
                break;
            }
        }
    }

    char statusMsg[100];
    sprintf(statusMsg, "Level 1 Writers: %d/%d", l1Writers, MAX_WRITERS_L1);
    info(statusMsg);

    sprintf(statusMsg, "Level 2 Aggregators: %d/1", l2Aggregators);
    info(statusMsg);

    sprintf(statusMsg, "Level 3 Readers: %d/%d", l3Readers, MAX_READERS_L3);
    info(statusMsg);

    info("\nData Flow Status:");
    if (l1Writers > 0)
    {
        info("  ✓ Level 1: Data being generated");
    }
    else
    {
        info("  ✗ Level 1: No data generation");
    }

    if (l2Aggregators > 0 && l1Writers > 0)
    {
        info("  ✓ Level 2: Data being aggregated");
    }
    else
    {
        info("  ✗ Level 2: No aggregation active");
    }

    if (l3Readers > 0 && l2Aggregators > 0)
    {
        info("  ✓ Level 3: Data being consumed");
    }
    else
    {
        info("  ✗ Level 3: No data consumption");
    }

    info("=============================================\n");
}

void displayMenu()
{
    info("\n============== MULTI-LEVEL SYSTEM MENU ==============");
    info("1. Start Complete 3-Level System (Recommended)");
    info("2. Launch L1 Writer (Level 1)");
    info("3. Launch L2 Aggregator (Level 2)");
    info("4. Launch L3 Reader (Level 3)");
    info("5. Display Active Processes");
    info("6. Display System Status");
    info("7. Terminate a Process");
    info("q. Exit and Cleanup All");
    info("===================================================");
}

void cleanupProcesses()
{
    for (int i = 0; i < processCount; i++)
    {
        if (processes[i].isActive)
        {
            terminate_process(processes[i].handle);
            close_process_handle(processes[i].handle);
            processes[i].isActive = false;
        }
    }
}
