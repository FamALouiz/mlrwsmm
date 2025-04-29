#ifndef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include "../../include/platform/sync.h"
#include "../../include/log/logger.h"

struct MutexHandle
{
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
    char *name;
    bool initialized;
};

struct SemaphoreHandle
{
    sem_t *sem;
    char *name;
};

// Mutex operations
MutexHandle *create_mutex(const char *name)
{
    MutexHandle *handle = (MutexHandle *)malloc(sizeof(MutexHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for mutex handle");
        return NULL;
    }

    handle->name = strdup(name);
    if (handle->name == NULL)
    {
        free(handle);
        error("Failed to allocate memory for mutex name");
        return NULL;
    }

    // Initialize mutex attributes
    if (pthread_mutexattr_init(&handle->attr) != 0)
    {
        free(handle->name);
        free(handle);
        error("Failed to initialize mutex attributes");
        return NULL;
    }

    // Make the mutex process-shared
    if (pthread_mutexattr_setpshared(&handle->attr, PTHREAD_PROCESS_SHARED) != 0)
    {
        pthread_mutexattr_destroy(&handle->attr);
        free(handle->name);
        free(handle);
        error("Failed to set mutex as process-shared");
        return NULL;
    }

    // Initialize the mutex
    if (pthread_mutex_init(&handle->mutex, &handle->attr) != 0)
    {
        pthread_mutexattr_destroy(&handle->attr);
        free(handle->name);
        free(handle);
        error("Failed to initialize mutex");
        return NULL;
    }

    handle->initialized = 1;
    return handle;
}

MutexHandle *open_mutex(const char *name)
{
    // In POSIX, there's no direct way to "open" an existing named mutex
    // The mutex would need to be in shared memory that we have access to
    // For this simplified example, we'll just create a new mutex with the same name
    return create_mutex(name);
}

bool lock_mutex(MutexHandle *handle)
{
    if (handle == NULL || !handle->initialized)
    {
        return 0;
    }

    return pthread_mutex_lock(&handle->mutex) == 0;
}

bool unlock_mutex(MutexHandle *handle)
{
    if (handle == NULL || !handle->initialized)
    {
        return 0;
    }

    return pthread_mutex_unlock(&handle->mutex) == 0;
}

bool close_mutex(MutexHandle *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    bool result = 1;

    if (handle->initialized)
    {
        if (pthread_mutex_destroy(&handle->mutex) != 0)
        {
            result = 0;
        }
        pthread_mutexattr_destroy(&handle->attr);
    }

    free(handle->name);
    free(handle);

    return result;
}

// Semaphore operations
SemaphoreHandle *create_semaphore(const char *name, long initial_count, long max_count)
{
    SemaphoreHandle *handle = (SemaphoreHandle *)malloc(sizeof(SemaphoreHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for semaphore handle");
        return NULL;
    }

    // POSIX semaphores should have names starting with a slash
    char *fullName = (char *)malloc(strlen(name) + 2);
    if (fullName == NULL)
    {
        free(handle);
        error("Failed to allocate memory for semaphore name");
        return NULL;
    }
    sprintf(fullName, "/%s", name);

    // Create the semaphore
    sem_t *sem = sem_open(fullName, O_CREAT | O_EXCL, 0644, initial_count);
    if (sem == SEM_FAILED)
    {
        // If semaphore already exists, try to open it
        sem = sem_open(fullName, 0);
        if (sem == SEM_FAILED)
        {
            char errorMsg[100];
            sprintf(errorMsg, "Could not create or open semaphore (%s).", fullName);
            error(errorMsg);
            free(fullName);
            free(handle);
            return NULL;
        }
    }

    handle->sem = sem;
    handle->name = fullName;

    return handle;
}

SemaphoreHandle *open_semaphore(const char *name)
{
    SemaphoreHandle *handle = (SemaphoreHandle *)malloc(sizeof(SemaphoreHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for semaphore handle");
        return NULL;
    }

    // POSIX semaphores should have names starting with a slash
    char *fullName = (char *)malloc(strlen(name) + 2);
    if (fullName == NULL)
    {
        free(handle);
        error("Failed to allocate memory for semaphore name");
        return NULL;
    }
    sprintf(fullName, "/%s", name);

    // Open the semaphore
    sem_t *sem = sem_open(fullName, 0);
    if (sem == SEM_FAILED)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not open semaphore (%s).", fullName);
        error(errorMsg);
        free(fullName);
        free(handle);
        return NULL;
    }

    handle->sem = sem;
    handle->name = fullName;

    return handle;
}

bool wait_semaphore(SemaphoreHandle *handle)
{
    if (handle == NULL || handle->sem == NULL)
    {
        return 0;
    }

    return sem_wait(handle->sem) == 0;
}

bool release_semaphore(SemaphoreHandle *handle, long release_count)
{
    if (handle == NULL || handle->sem == NULL)
    {
        return 0;
    }

    bool result = 1;
    for (long i = 0; i < release_count; i++)
    {
        if (sem_post(handle->sem) != 0)
        {
            result = 0;
            break;
        }
    }

    return result;
}

bool close_semaphore(SemaphoreHandle *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    bool result = 1;

    if (sem_close(handle->sem) != 0)
    {
        result = 0;
    }

    // Note: We don't call sem_unlink here since that would delete
    // the semaphore, which we only want to do when the last process is done

    free(handle->name);
    free(handle);

    return result;
}

// Platform-independent wait function
void platform_sleep(unsigned int milliseconds)
{
    usleep(milliseconds * 1000); // usleep takes microseconds
}

// For non-blocking keyboard input on POSIX systems
// Variables to keep track of terminal state
static struct termios orig_term_attr;
static int term_initialized = 0;

// Initialize terminal for non-blocking input
static void init_terminal()
{
    if (!term_initialized)
    {
        tcgetattr(STDIN_FILENO, &orig_term_attr);
        struct termios new_term_attr = orig_term_attr;
        new_term_attr.c_lflag &= ~(ICANON | ECHO);
        new_term_attr.c_cc[VTIME] = 0;
        new_term_attr.c_cc[VMIN] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_term_attr);
        term_initialized = 1;
    }
}

// Restore terminal to original state
static void restore_terminal()
{
    if (term_initialized)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_term_attr);
        term_initialized = 0;
    }
}

// Platform-independent keyboard input functions
bool kbhit()
{
    init_terminal();

    fd_set set;
    struct timeval tv = {0, 0};
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    int result = select(STDIN_FILENO + 1, &set, NULL, NULL, &tv);

    return result > 0;
}

int getch()
{
    init_terminal();

    char c;
    if (read(STDIN_FILENO, &c, 1) < 1)
    {
        return EOF;
    }

    return c;
}

#endif // !_WIN32