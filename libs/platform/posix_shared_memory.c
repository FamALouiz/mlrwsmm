#ifndef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../include/platform/shared_memory.h"
#include "../../include/log/logger.h"

struct SharedMemoryHandle
{
    int fd;
    char *name;
    size_t size;
};

SharedMemoryHandle *create_shared_memory(const char *name, size_t size)
{
    SharedMemoryHandle *handle = (SharedMemoryHandle *)malloc(sizeof(SharedMemoryHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for shared memory handle");
        return NULL;
    }

    // POSIX shared memory objects start with a slash
    char *fullName = (char *)malloc(strlen(name) + 2);
    if (fullName == NULL)
    {
        free(handle);
        error("Failed to allocate memory for shared memory name");
        return NULL;
    }
    sprintf(fullName, "/%s", name);

    // Create the shared memory object
    int fd = shm_open(fullName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not create shared memory object (%s).", fullName);
        error(errorMsg);
        free(fullName);
        free(handle);
        return NULL;
    }

    // Set the size of the shared memory object
    if (ftruncate(fd, size) == -1)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not set size of shared memory object (%s).", fullName);
        error(errorMsg);
        close(fd);
        shm_unlink(fullName);
        free(fullName);
        free(handle);
        return NULL;
    }

    handle->fd = fd;
    handle->name = fullName;
    handle->size = size;

    return handle;
}

SharedMemoryHandle *open_shared_memory(const char *name)
{
    SharedMemoryHandle *handle = (SharedMemoryHandle *)malloc(sizeof(SharedMemoryHandle));
    if (handle == NULL)
    {
        error("Failed to allocate memory for shared memory handle");
        return NULL;
    }

    // POSIX shared memory objects start with a slash
    char *fullName = (char *)malloc(strlen(name) + 2);
    if (fullName == NULL)
    {
        free(handle);
        error("Failed to allocate memory for shared memory name");
        return NULL;
    }
    sprintf(fullName, "/%s", name);

    // Open the shared memory object
    int fd = shm_open(fullName, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        char errorMsg[100];
        sprintf(errorMsg, "Could not open shared memory object (%s).", fullName);
        error(errorMsg);
        free(fullName);
        free(handle);
        return NULL;
    }

    // Determine the size of the shared memory object
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        close(fd);
        free(fullName);
        free(handle);
        error("Could not determine size of shared memory object.");
        return NULL;
    }

    handle->fd = fd;
    handle->name = fullName;
    handle->size = sb.st_size;

    return handle;
}

void *map_shared_memory(SharedMemoryHandle *handle, size_t size)
{
    if (handle == NULL)
    {
        return NULL;
    }

    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);

    if (data == MAP_FAILED)
    {
        error("Could not map shared memory.");
        return NULL;
    }

    return data;
}

bool unmap_shared_memory(void *data)
{
    if (data == NULL)
    {
        return 0;
    }

    // We need to know the size to unmap, which we don't have here.
    // This is a design limitation. For POSIX, we'd usually need to track
    // both the pointer and the size.
    // For now, we assume the shared memory size is SHARED_MEM_SIZE
    return munmap(data, SHARED_MEM_SIZE) == 0;
}

bool close_shared_memory(SharedMemoryHandle *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    bool result = 1;

    if (close(handle->fd) == -1)
    {
        result = 0;
    }

    // Note: We don't call shm_unlink here because that would actually
    // delete the shared memory object, which we only want to do when
    // the last process is done with it.

    free(handle->name);
    free(handle);

    return result;
}

#endif // !_WIN32