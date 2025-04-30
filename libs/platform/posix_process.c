#ifndef _WIN32

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "../../include/platform/process.h"
#include "../../include/log/logger.h"

struct ProcessHandle
{
    pid_t pid;
    int id;
    char type;
    bool isActive;
};

bool create_process(const char *command, int id, char process_type, ProcessHandle **handle)
{
    // Allocate memory for the process handle
    *handle = (ProcessHandle *)malloc(sizeof(ProcessHandle));
    if (*handle == NULL)
    {
        error("Failed to allocate memory for process handle");
        return 0;
    }

    // Parse the command into arguments
    char *cmd_copy = strdup(command);
    char *argv[64] = {NULL}; // Assuming max 64 arguments
    int argc = 0;

    char *token = strtok(cmd_copy, " ");
    while (token != NULL && argc < 63)
    {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;

    // Fork a new process
    pid_t pid = fork();

    if (pid < 0)
    {
        // Fork failed
        char errorMsg[100];
        sprintf(errorMsg, "Fork failed for %c %d", process_type, id);
        error(errorMsg);
        free(cmd_copy);
        free(*handle);
        *handle = NULL;
        return 0;
    }
    else if (pid == 0)
    {
        // Child process
        execvp(argv[0], argv);

        // If execvp returns, it means it failed
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        (*handle)->pid = pid;
        (*handle)->id = id;
        (*handle)->type = process_type;
        (*handle)->isActive = 1;

        char successMsg[100];
        sprintf(successMsg, "%c %d started successfully with PID %d.", process_type, id, pid);
        info(successMsg);

        free(cmd_copy);
        return 1;
    }
}

bool terminate_process(ProcessHandle *handle)
{
    if (handle == NULL || !handle->isActive)
    {
        return 0;
    }

    if (kill(handle->pid, SIGTERM) == 0)
    {
        handle->isActive = 0;
        return 1;
    }

    return 0;
}

void close_process_handle(ProcessHandle *handle)
{
    if (handle != NULL)
    {
        // In POSIX, we don't need to explicitly close handles
        // But we should wait for the process to avoid zombies if it's terminated
        if (handle->isActive)
        {
            waitpid(handle->pid, NULL, WNOHANG);
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
    return (unsigned long)handle->pid;
}

bool is_process_active(ProcessHandle *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    if (kill(handle->pid, 0) == 0)
    {
        // Process exists
        int status;
        pid_t result = waitpid(handle->pid, &status, WNOHANG);

        if (result == 0)
        {
            // Process is still running
            return 1;
        }
    }

    // Process does not exist or has terminated
    handle->isActive = 0;
    return 0;
}

#endif // !_WIN32