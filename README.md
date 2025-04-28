# Multi-level Reader-Writer Synchronization and Memory Management (mlrwsmm)

This project implements a multi-level reader-writer synchronization system using shared memory for interprocess communication under Windows. The implementation allows for multiple readers and writers to access shared data with configurable priority. This is a classic synchronization problem in operating systems where multiple processes need to access shared resources with different access levels.

## Features

-   Shared memory communication between processes
-   Support for multiple concurrent readers
-   Exclusive access for writers
-   Dynamic priority toggling between reader-priority and writer-priority modes
-   Process management through a central controller application
-   Custom logging system with configurable verbosity levels

## Architecture

The system consists of three main components:

1. **Main Controller**: Manages reader and writer processes, provides UI for toggling priorities and launching processes
2. **Reader Processes**: Multiple readers can access shared data simultaneously when no writers are active
3. **Writer Processes**: Writers get exclusive access to shared data, blocking all other processes

## Priority Modes

The system supports two priority modes that can be toggled at runtime:

-   **Writer Priority**: Writers get precedence over readers. If any writers are waiting, new readers will wait.
-   **Reader Priority**: Readers get precedence over writers. Writers must wait until all readers have finished.

## Getting Started

### Prerequisites

-   Windows operating system
-   C compiler (Microsoft Visual C++ recommended)
-   CMake (version 3.15 or higher)

### Building

There are two ways to build the project:

#### Option 1: Using Provided Scripts

1. Clone the repository
2. Run one of the build scripts:
    - For Windows: `scripts/run.bat`
    - For Unix-like systems: `scripts/run.sh` (make sure it has execute permissions)

These scripts will create a build directory, run CMake, build the project, and then run the application automatically.

#### Option 2: Manual Build

1. Clone the repository
2. Create a build directory: `mkdir build && cd build`
3. Run CMake: `cmake ..`
4. Build the project: `cmake --build .`
5. Run the main executable from the project root: `./build/bin/main.exe`

### Running

1. Run the main.exe executable (located in `build/bin/main.exe` after building)
2. The controller will automatically launch 2 writer processes and 2 reader processes
3. Use the menu options to:
    - Launch additional reader/writer processes
    - Toggle between reader and writer priority
    - View active processes
    - Terminate specific processes
    - Exit the application

## Implementation Details

### Synchronization Mechanism

The synchronization between readers and writers is achieved using:

-   Windows File Mapping for shared memory
-   Mutexes for protecting critical sections
-   Semaphores for controlling access based on priority

### Logging System

The project includes a custom logging system with:

-   Different verbosity levels (INFO, DEBUG)
-   Multiple output options (terminal, file, or both)
-   Timestamps relative to program start

## Project Structure

```
README.md              # This documentation file
CMakeLists.txt         # CMake build configuration
include/
  common.h             # Common definitions for shared memory structures
  log/
    logger.h           # Logger interface
    logger_config.h    # Logger configuration definitions
  path/
    path.h             # Path utilities header
libs/
  log/
    logger.c           # Logger implementation
  reader/
    reader.c           # Reader process implementation
  util/
    path.c             # Path utilities implementation
  writer/
    writer.c           # Writer process implementation
scripts/
  run.bat              # Windows build and run script
  run.sh               # Unix build and run script
src/
  main.c               # Main controller application
build/                 # Generated build files (after compilation)
  bin/                 # Compiled executables
    main.exe           # Main controller executable
    reader.exe         # Reader process executable
    writer.exe         # Writer process executable
```

## How It Works

1. The main controller creates shared memory and synchronization objects
2. It spawns reader and writer processes which connect to the shared memory
3. Readers can access the shared data simultaneously when no writers are active
4. Writers get exclusive access, blocking all other processes
5. The priority mode determines whether readers or writers get precedence when both are waiting
6. The system demonstrates classic IPC (Inter-Process Communication) and synchronization techniques

## Dependencies

The project uses only standard Windows API and C standard libraries, with no external dependencies.
