#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include "../../include/platform/process.h"
#include "../../include/log/logger.h"

struct ProcessHandle
{
    HANDLE hProcess;
    HANDLE hThread;
    int id;
    char type;
    bool isActive;
};

bool create_process(const char *command, int id, char process_type, ProcessHandle **handle)
{
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi;

    // Allocate memory for the process handle
    *handle = (ProcessHandle *)malloc(sizeof(ProcessHandle));
    if (*handle == NULL)
    {
        error("Failed to allocate memory for process handle");
        return 0;
    }

    // Create the process
    if (!CreateProcess(NULL, (LPSTR)command, NULL, NULL, FALSE,
                       CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
    {
        char errorMsg[100];
        sprintf(errorMsg, "CreateProcess failed for %c %d (%lu).", process_type, id, GetLastError());
        error(errorMsg);
        free(*handle);
        *handle = NULL;
        return 0;
    }

    // Store process information
    (*handle)->hProcess = pi.hProcess;
    (*handle)->hThread = pi.hThread;
    (*handle)->id = id;
    (*handle)->type = process_type;
    (*handle)->isActive = 1;

    char successMsg[100];
    sprintf(successMsg, "%c %d started successfully.", process_type, id);
    info(successMsg);
    return 1;
}

bool terminate_process(ProcessHandle *handle)
{
    if (handle == NULL || !handle->isActive)
    {
        return 0;
    }

    bool result = TerminateProcess(handle->hProcess, 0);
    if (result)
    {
        handle->isActive = 0;
    }
    return result;
}

void close_process_handle(ProcessHandle *handle)
{
    if (handle != NULL)
    {
        if (handle->hProcess != NULL)
        {
            CloseHandle(handle->hProcess);
        }
        if (handle->hThread != NULL)
        {
            CloseHandle(handle->hThread);
        }
        free(handle);
    }
}

unsigned long get_process_id(ProcessHandle *handle)
{
    if (handle == NULL || !handle->isActive)
    {
        return 0;
    }
    return GetProcessId(handle->hProcess);
}

bool is_process_active(ProcessHandle *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    DWORD exitCode;
    if (GetExitCodeProcess(handle->hProcess, &exitCode))
    {
        return exitCode == STILL_ACTIVE;
    }
    return 0;
}

#endif // _WIN32