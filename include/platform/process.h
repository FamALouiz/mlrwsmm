#ifndef PLATFORM_PROCESS_H
#define PLATFORM_PROCESS_H

#include <stdbool.h>

// Process management structure
typedef struct ProcessHandle ProcessHandle;

// Process creation and management
bool create_process(const char *command, int id, char process_type, ProcessHandle **handle);
bool terminate_process(ProcessHandle *handle);
void close_process_handle(ProcessHandle *handle);
unsigned long get_process_id(ProcessHandle *handle);
bool is_process_active(ProcessHandle *handle);

#endif // PLATFORM_PROCESS_H