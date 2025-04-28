#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "..\include\common.h"
#include "..\include\log\logger.h"

#define MAX_COMMAND_LENGTH 256
#define NUM_WRITERS 2
#define NUM_READERS 2

// Process management structure
typedef struct
{
    HANDLE hProcess;
    HANDLE hThread;
    int id;
    char type; // 'R' for reader, 'W' for writer
    bool isActive;
} ProcessInfo;

// Global array to store process information
ProcessInfo processes[NUM_WRITERS + NUM_READERS];
int processCount = 0;

// Function prototypes
bool createReaderProcess(int readerId);
bool createWriterProcess(int writerId);
void displayMenu();
void cleanupProcesses();
void togglePriority();

int main()
{
    // Initialize the logger with terminal output and INFO verbosity
    init_logger(LOG_TO_TERMINAL_ONLY, LOG_VERBOSITY_INFO);

    info("Multi-level Reader-Writer Synchronization and Memory Management");
    info("==============================================================");

    // Initialize the processes array
    for (int i = 0; i < NUM_WRITERS + NUM_READERS; i++)
    {
        processes[i].isActive = false;
    }

    // Create initial writer processes - we need at least one writer to initialize shared memory
    info("Creating Writer 1 to initialize shared memory...");
    if (!createWriterProcess(1))
    {
        error("Failed to create initial writer process. Exiting.");
        return 1;
    }

    // Create the rest of the processes
    info("Creating Writer 2...");
    createWriterProcess(2);

    info("Creating Reader 1...");
    createReaderProcess(1);

    info("Creating Reader 2...");
    createReaderProcess(2);

    // Main menu loop
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
            // Launch a new reader process
            {
                int id;
                info("Enter reader ID: ");
                scanf("%d", &id);
                createReaderProcess(id);
            }
            break;

        case '2':
            // Launch a new writer process
            {
                int id;
                info("Enter writer ID: ");
                scanf("%d", &id);
                createWriterProcess(id);
            }
            break;

        case '3':
            // Toggle priority mode
            togglePriority();
            break;

        case '4':
            // Display active processes
            info("\nActive Processes:");
            for (int i = 0; i < processCount; i++)
            {
                if (processes[i].isActive)
                {
                    char processInfo[100];
                    sprintf(processInfo, "%c Process %d (PID: %lu)",
                            processes[i].type, processes[i].id,
                            GetProcessId(processes[i].hProcess));
                    info(processInfo);
                }
            }
            info("");
            break;

        case '5':
            // Terminate a specific process
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

                    TerminateProcess(processes[index].hProcess, 0);
                    CloseHandle(processes[index].hProcess);
                    CloseHandle(processes[index].hThread);
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

    // Cleanup
    cleanupProcesses();
    info("All processes terminated. Goodbye!");

    // Close the logger before exiting
    close_logger();

    return 0;
}

bool createReaderProcess(int readerId)
{
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    // Build command line with reader ID
    char command[MAX_COMMAND_LENGTH];
    sprintf_s(command, MAX_COMMAND_LENGTH, "build\\bin\\reader.exe %d", readerId);

    // Create the reader process
    if (!CreateProcess(NULL, command, NULL, NULL, FALSE,
                       CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
    {
        char errorMsg[100];
        sprintf(errorMsg, "CreateProcess failed for reader %d (%lu).", readerId, GetLastError());
        error(errorMsg);
        return false;
    }

    // Store process information
    processes[processCount].hProcess = pi.hProcess;
    processes[processCount].hThread = pi.hThread;
    processes[processCount].id = readerId;
    processes[processCount].type = 'R';
    processes[processCount].isActive = true;
    processCount++;

    char successMsg[100];
    sprintf(successMsg, "Reader %d started successfully.", readerId);
    info(successMsg);
    return true;
}

bool createWriterProcess(int writerId)
{
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    // Build command line with writer ID
    char command[MAX_COMMAND_LENGTH];
    sprintf_s(command, MAX_COMMAND_LENGTH, "build\\bin\\writer.exe %d", writerId);

    // Create the writer process
    if (!CreateProcess(NULL, command, NULL, NULL, FALSE,
                       CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
    {
        char errorMsg[100];
        sprintf(errorMsg, "CreateProcess failed for writer %d (%lu).", writerId, GetLastError());
        error(errorMsg);
        return false;
    }

    // Store process information
    processes[processCount].hProcess = pi.hProcess;
    processes[processCount].hThread = pi.hThread;
    processes[processCount].id = writerId;
    processes[processCount].type = 'W';
    processes[processCount].isActive = true;
    processCount++;

    char successMsg[100];
    sprintf(successMsg, "Writer %d started successfully.", writerId);
    info(successMsg);
    return true;
}

void togglePriority()
{
    // Open shared memory to access the priority flag
    HANDLE hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,     // Read/write access
        FALSE,                   // Do not inherit the name
        TEXT(SHARED_MEMORY_NAME) // Name of mapping object
    );

    if (hMapFile == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not open file mapping object (%lu).", GetLastError());
        error(errorMsg);
        warn("Make sure at least one writer or reader is running.");
        return;
    }

    // Map the shared memory
    SharedData *sharedData = (SharedData *)MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEM_SIZE);

    if (sharedData == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not map view of file (%lu).", GetLastError());
        error(errorMsg);
        CloseHandle(hMapFile);
        return;
    }

    // Open priority mutex
    HANDLE priorityMutex = OpenMutex(
        SYNCHRONIZE,              // Access right
        FALSE,                    // Don't inherit
        TEXT(PRIORITY_MUTEX_NAME) // Name
    );

    if (priorityMutex == NULL)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not open priority mutex (%lu).", GetLastError());
        error(errorMsg);
        UnmapViewOfFile(sharedData);
        CloseHandle(hMapFile);
        return;
    }

    // Toggle the priority mode
    WaitForSingleObject(priorityMutex, INFINITE);
    sharedData->isPriorityWriter = !sharedData->isPriorityWriter;

    char priorityMsg[100];
    sprintf(priorityMsg, "Priority mode switched to %s priority.",
            sharedData->isPriorityWriter ? "WRITER" : "READER");
    info(priorityMsg);

    ReleaseMutex(priorityMutex);

    // Clean up
    UnmapViewOfFile(sharedData);
    CloseHandle(priorityMutex);
    CloseHandle(hMapFile);
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
            TerminateProcess(processes[i].hProcess, 0);
            CloseHandle(processes[i].hProcess);
            CloseHandle(processes[i].hThread);
        }
    }
}
