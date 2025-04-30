#ifndef PLATFORM_SHARED_MEMORY_H
#define PLATFORM_SHARED_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include "../common.h"

// Shared memory handle structure
typedef struct SharedMemoryHandle SharedMemoryHandle;

// Shared memory operations
SharedMemoryHandle *create_shared_memory(const char *name, size_t size);
SharedMemoryHandle *open_shared_memory(const char *name);
void *map_shared_memory(SharedMemoryHandle *handle, size_t size);
bool unmap_shared_memory(void *data);
bool close_shared_memory(SharedMemoryHandle *handle);

#endif // PLATFORM_SHARED_MEMORY_H