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
#define NUM_WRITERS 2
#define NUM_READERS 2

typedef struct
{
    ProcessHandle *handle;
    int id;
    char type; // 'R' for reader, 'W' for writer
    bool isActive;
} ProcessInfo;

ProcessInfo processes[NUM_WRITERS + NUM_READERS];
int processCount = 0;

bool createReaderProcess(int readerId);
bool createWriterProcess(int writerId);
void displayMenu();
void cleanupProcesses();
void togglePriority();

int main()
{
    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    info("Multi-level Reader-Writer Synchronization and Memory Management");
    info("==============================================================");

    for (int i = 0; i < NUM_WRITERS + NUM_READERS; i++)
    {
        processes[i].isActive = 0;
    }

    info("Creating Writer 1 to initialize shared memory...");
    if (!createWriterProcess(1))
    {
        error("Failed to create initial writer process. Exiting.");
        return 1;
    }

    for (int i = 2; i <= NUM_WRITERS; i++)
    {
        char writerMsg[100];
        sprintf(writerMsg, "Creating Writer %d...", i);
        info(writerMsg);
        createWriterProcess(i);
    }

    for (int i = 1; i <= NUM_READERS; i++)
    {
        char readerMsg[100];
        sprintf(readerMsg, "Creating Reader %d...", i);
        info(readerMsg);
        createReaderProcess(i);
    }

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
        {
            int id;
            info("Enter reader ID: ");
            scanf("%d", &id);
            createReaderProcess(id);
        }
        break;

        case '2':
        {
            int id;
            info("Enter writer ID: ");
            scanf("%d", &id);
            createWriterProcess(id);
        }
        break;

        case '3':
            togglePriority();
            break;

        case '4':
            info("\nActive Processes:");
            for (int i = 0; i < processCount; i++)
            {
                if (processes[i].isActive)
                {
                    char processInfo[100];
                    sprintf(processInfo, "%c Process %d (PID: %lu)",
                            processes[i].type, processes[i].id,
                            get_process_id(processes[i].handle));
                    info(processInfo);
                }
            }
            info("");
            break;

        case '5':
        {
            int index;
            info("Enter process index to terminate (from list): ");
            scanf("%d", &index);

            if (index >= 0 && index < processCount && processes[index].isActive)
            {
                char terminateMsg[100];
                sprintf(terminateMsg, "Terminating %c Process %d...",
                        processes[index].type, processes[index].id);
                info(terminateMsg);

                terminate_process(processes[index].handle);
                close_process_handle(processes[index].handle);
                processes[index].isActive = 0;

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
            running = 0;
            break;

        default:
            warn("Invalid choice. Please try again.");
        }
    }

    cleanupProcesses();
    info("All processes terminated. Goodbye!");

    close_logger();

    return 0;
}

bool createReaderProcess(int readerId)
{
    char command[MAX_COMMAND_LENGTH];

    const char *pathParts[] = {"build", "bin", "reader.exe"};
    char exePath[MAX_COMMAND_LENGTH];
    join_paths(exePath, 3, pathParts);

    sprintf(command, "%s %d", exePath, readerId);

    ProcessHandle *handle = NULL;
    if (!create_process(command, readerId, 'R', &handle))
    {
        char errorMsg[100];
        sprintf(errorMsg, "Process creation failed for reader %d.", readerId);
        error(errorMsg);
        return 0;
    }

    processes[processCount].handle = handle;
    processes[processCount].id = readerId;
    processes[processCount].type = 'R';
    processes[processCount].isActive = 1;
    processCount++;

    char successMsg[100];
    sprintf(successMsg, "Reader %d started successfully.", readerId);
    info(successMsg);
    return 1;
}

bool createWriterProcess(int writerId)
{
    char command[MAX_COMMAND_LENGTH];

    const char *pathParts[] = {"build", "bin", "writer.exe"};
    char exePath[MAX_COMMAND_LENGTH];
    join_paths(exePath, 3, pathParts);

    sprintf(command, "%s %d", exePath, writerId);

    ProcessHandle *handle = NULL;
    if (!create_process(command, writerId, 'W', &handle))
    {
        char errorMsg[100];
        sprintf(errorMsg, "Process creation failed for writer %d.", writerId);
        error(errorMsg);
        return 0;
    }

    processes[processCount].handle = handle;
    processes[processCount].id = writerId;
    processes[processCount].type = 'W';
    processes[processCount].isActive = 1;
    processCount++;

    char successMsg[100];
    sprintf(successMsg, "Writer %d started successfully.", writerId);
    info(successMsg);
    return 1;
}

void togglePriority()
{
    SharedMemoryHandle *hMapFile = open_shared_memory(SHARED_MEMORY_NAME);
    if (hMapFile == NULL)
    {
        error("Could not open shared memory object.");
        warn("Make sure at least one writer or reader is running.");
        return;
    }

    SharedData *sharedData = (SharedData *)map_shared_memory(hMapFile, SHARED_MEM_SIZE);
    if (sharedData == NULL)
    {
        error("Could not map shared memory.");
        close_shared_memory(hMapFile);
        return;
    }

    MutexHandle *priorityMutex = open_mutex(PRIORITY_MUTEX_NAME);
    if (priorityMutex == NULL)
    {
        error("Could not open priority mutex.");
        unmap_shared_memory(sharedData);
        close_shared_memory(hMapFile);
        return;
    }

    lock_mutex(priorityMutex);
    sharedData->isPriorityWriter = !sharedData->isPriorityWriter;

    char priorityMsg[100];
    sprintf(priorityMsg, "Priority mode switched to %s priority.",
            sharedData->isPriorityWriter ? "WRITER" : "READER");
    info(priorityMsg);

    unlock_mutex(priorityMutex);

    close_mutex(priorityMutex);
    unmap_shared_memory(sharedData);
    close_shared_memory(hMapFile);
}

void displayMenu()
{
    info("\n------ Menu ------");
    info("1. Launch a new reader");
    info("2. Launch a new writer");
    info("3. Toggle priority mode (reader/writer)");
    info("4. Display active processes");
    info("5. Terminate a process");
    info("q. Exit");
}

void cleanupProcesses()
{
    for (int i = 0; i < processCount; i++)
    {
        if (processes[i].isActive)
        {
            terminate_process(processes[i].handle);
            close_process_handle(processes[i].handle);
            processes[i].isActive = 0;
        }
    }
}
