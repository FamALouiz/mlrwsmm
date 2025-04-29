cmake_minimum_required(VERSION 3.10)
project(mlrwsmm C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Define include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Define output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set output directories for each target type
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# Determine platform and set platform-specific files
if(WIN32)
    set(PLATFORM_SOURCES
        libs/platform/win_process.c
        libs/platform/win_shared_memory.c
        libs/platform/win_sync.c
    )
    add_definitions(-D_WIN32)
else()
    set(PLATFORM_SOURCES
        libs/platform/posix_process.c
        libs/platform/posix_shared_memory.c
        libs/platform/posix_sync.c
    )
    # Add necessary POSIX libraries
    set(PLATFORM_LIBS pthread rt)
endif()

# Create platform abstraction library
add_library(platform STATIC ${PLATFORM_SOURCES})

# Create logger library
add_library(logger STATIC 
    libs/log/logger.c
    libs/util/path.c
)

# Define reader target
add_executable(reader libs/reader/reader.c)
set_target_properties(reader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Define writer target
add_executable(writer libs/writer/writer.c)
set_target_properties(writer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Define main target
add_executable(main src/main.c)

# Link libraries
target_link_libraries(reader logger platform ${PLATFORM_LIBS})
target_link_libraries(writer logger platform ${PLATFORM_LIBS})
target_link_libraries(main logger platform ${PLATFORM_LIBS})

# Install rules
install(TARGETS reader writer main
        RUNTIME DESTINATION bin)