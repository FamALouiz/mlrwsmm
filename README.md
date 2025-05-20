# Multi-level Reader-Writer Synchronization and Memory Management (mlrwsmm)

This project implements a multi-level reader-writer synchronization system using shared memory for interprocess communication. The implementation allows for multiple readers and writers to access shared data with configurable priority. This is a classic synchronization problem in operating systems where multiple processes need to access shared resources with different access levels.

## Features

-   Cross-platform implementation (Windows and POSIX systems like Linux, macOS)
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

-   C compiler (GCC, Clang, or MSVC)
-   CMake (version 3.10 or higher)
-   For Windows: Windows 10/11 with Visual Studio or MinGW
-   For Linux/macOS: Development tools including pthread and rt libraries

### Building

There are two ways to build the project:

#### Option 1: Using Provided Scripts

1. Clone the repository
2. Run one of the build scripts:
    - For Windows: `scripts/run.bat`
    - For Unix-like systems: `scripts/run.sh` (make sure it has execute permissions: `chmod +x scripts/run.sh`)

These scripts will create a build directory, run CMake, build the project, and then run the application automatically.

#### Option 2: Manual Build

1. Clone the repository
2. Create a build directory: `mkdir build && cd build`
3. Run CMake: `cmake ..`
4. Build the project:
    - Windows: `cmake --build . --config Debug`
    - Linux/macOS: `cmake --build .`
5. Run the main executable from the build directory:
    - Windows: `.\bin\main.exe`
    - Linux/macOS: `./bin/main`

### Running

1. Run the main executable (located in `build/bin/` after building) [If the scripts were used then this will run automatically]
2. The controller will automatically launch 2 writer processes and 2 reader processes
3. Use the menu options to:
    - Launch additional reader/writer processes
    - Toggle between reader and writer priority
    - View active processes
    - Terminate specific processes
    - Exit the application

## Implementation Details

### Cross-Platform Abstraction Layer

The project uses a platform abstraction layer that provides a consistent interface across different operating systems:

-   **Windows Implementation**: Uses Win32 API functions for process management, shared memory, and synchronization
-   **POSIX Implementation**: Uses pthread, semaphores, and shared memory facilities for Linux, macOS, and other UNIX-like systems

The abstraction layer covers:

-   Process creation and management
-   Shared memory operations
-   Synchronization primitives (mutexes and semaphores)
-   Platform-specific keyboard input handling

### Synchronization Mechanism

The synchronization between readers and writers is achieved using:

-   Platform-specific shared memory mechanisms (Windows File Mapping or POSIX shared memory)
-   Mutexes for protecting critical sections
-   Semaphores for controlling access based on priority

### Logging System

The project includes a custom logging system with:

-   Different verbosity levels (ERROR, WARNING, INFO, DEBUG)
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
  memory/
    memory_manager.h   # Memory management system interface
  path/
    path.h             # Path utilities header
  platform/
    process.h          # Platform-independent process management
    shared_memory.h    # Platform-independent shared memory operations
    sync.h             # Platform-independent synchronization primitives
libs/
  log/
    logger.c           # Logger implementation
  memory/
    memory_manager.c   # Memory management system implementation
  platform/
    posix_process.c    # POSIX implementation of process management
    posix_shared_memory.c # POSIX implementation of shared memory
    posix_sync.c       # POSIX implementation of synchronization
    win_process.c      # Windows implementation of process management
    win_shared_memory.c # Windows implementation of shared memory
    win_sync.c         # Windows implementation of synchronization
  reader/
    reader.c           # Reader process implementation
  util/
    path.c             # Path utilities implementation
  writer/
    writer.c           # Writer process implementation
scripts/
  memory_simulator.bat # Windows memory simulator script
  memory_simulator.sh  # Unix memory simulator script
  run.bat              # Windows build and run script
  run.sh               # Unix build and run script
src/
  main.c               # Main controller application
  memory_simulator.c   # Memory management simulator application
build/                 # Generated build files (after compilation)
  bin/                 # Compiled executables
    main.exe           # Main controller executable
    memory_simulator.exe # Memory simulator executable
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

## Platform-Specific Considerations

### Windows

-   Uses Win32 API for process creation, shared memory, and synchronization
-   Executables have `.exe` extension
-   Process creation uses `CreateProcess`

### Linux/macOS/UNIX

-   Uses POSIX APIs (pthread, semaphores, shared memory)
-   Requires pthread and rt libraries
-   Process creation uses `fork` and `execvp`
-   Named semaphores and shared memory objects have `/` prefix in their names

## Dependencies

-   Windows implementation: Standard Windows API (kernel32.lib)
-   POSIX implementation: pthread and rt libraries (linked automatically by CMake)
-   Standard C library

## Memory Management System

The project also includes a sophisticated memory management simulation system that demonstrates common operating system memory allocation strategies:

### Memory Allocation Strategies

The system supports three memory allocation strategies:

-   **Segmentation**: Memory is divided into variable-sized segments
-   **Paging**: Memory is divided into fixed-size pages
-   **Hybrid**: A combination of both segmentation and paging approaches

### Memory Manager Features

-   Process-based memory allocation and deallocation
-   Segment management with first-fit allocation algorithm
-   Page table management for virtual-to-physical address translation
-   Memory fragmentation analysis (internal and external)
-   Memory visualization tools for debugging and educational purposes

### Memory Simulator

A separate memory simulator application (`memory_simulator.exe`) is included that demonstrates the memory management system:

1. Launch the memory simulator using:

    - Windows: `scripts/memory_simulator.bat`
    - Unix-like systems: `scripts/memory_simulator.sh`

2. The simulator allows you to:
    - Create processes with different memory requirements
    - Allocate and deallocate memory using different strategies
    - Visualize memory usage and fragmentation in real-time
    - Compare the efficiency of different memory allocation strategies

### Implementation Details

-   Custom memory segment and page data structures
-   Process tracking with memory allocation information
-   Efficient memory allocation and deallocation algorithms
-   Fragmentation analysis and statistics
-   Visual memory map representation
