#ifndef PLATFORM_SYNC_H
#define PLATFORM_SYNC_H

#include <stdbool.h>
#include <stddef.h>

// Handle types for synchronization primitives
typedef struct MutexHandle MutexHandle;
typedef struct SemaphoreHandle SemaphoreHandle;

// Mutex operations
MutexHandle *create_mutex(const char *name);
MutexHandle *open_mutex(const char *name);
bool lock_mutex(MutexHandle *handle);
bool unlock_mutex(MutexHandle *handle);
bool close_mutex(MutexHandle *handle);

// Semaphore operations
SemaphoreHandle *create_semaphore(const char *name, long initial_count, long max_count);
SemaphoreHandle *open_semaphore(const char *name);
bool wait_semaphore(SemaphoreHandle *handle);
bool release_semaphore(SemaphoreHandle *handle, long release_count);
bool close_semaphore(SemaphoreHandle *handle);

// Platform-independent wait function
void platform_sleep(unsigned int milliseconds);

// Platform-independent keyboard input functions
int getch();

#endif // PLATFORM_SYNC_H