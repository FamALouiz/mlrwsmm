#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include "logger_config.h"

#define warn(message) log_msg(LOG_STATUS_LEVEL_WARNING, message, __FILE__, __LINE__);
#define info(message) log_msg(LOG_STATUS_LEVEL_NORMAL, message, __FILE__, __LINE__);
#define error(message) log_msg(LOG_STATUS_LEVEL_ERROR, message, __FILE__, __LINE__);

// Global variables
extern int log_verbosity;
extern int log_output;
extern FILE *log_file;
extern time_t init_time;

// Function declarations
int init_logger(int log_output_code, int log_verbosity_code);
void close_logger();

#endif