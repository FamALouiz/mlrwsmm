#include <stdio.h>
#include <string.h>
#include "../include/path/path.h"

void join_paths(char *result, int num_paths, const char *paths[])
{
    result[0] = '\0'; // Initialize result as an empty string

    for (int i = 0; i < num_paths; i++)
    {
        if (i > 0)
        {
            size_t len = strlen(result);
            if (len > 0 && result[len - 1] != PATH_SEPARATOR[0])
            {
                strcat(result, PATH_SEPARATOR);
            }
        }
        strcat(result, paths[i]);
    }
}

const char *get_last_file_in_path(const char *path)
{
    const char *last_separator = strrchr(path, '/'); // For Unix-like systems
#ifdef _WIN32
    const char *last_backslash = strrchr(path, '\\'); // For Windows systems
    if (last_backslash && (!last_separator || last_backslash > last_separator))
    {
        last_separator = last_backslash;
    }
#endif
    return last_separator ? last_separator + 1 : path; // Return the file name or the full path if no separator is found
}