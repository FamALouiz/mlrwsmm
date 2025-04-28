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

# Create logger library
add_library(logger STATIC 
    libs/log/logger.c
    libs/util/path.c
)

# Define reader target
add_executable(reader libs/reader/reader.c)
set_target_properties(reader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/reader
)

# Define writer target
add_executable(writer libs/writer/writer.c)
set_target_properties(writer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/writer
)

# Define main target
add_executable(main src/main.c)

# Add Windows-specific libraries
target_link_libraries(reader logger)
target_link_libraries(writer logger)
target_link_libraries(main logger)

# Install rules
install(TARGETS reader writer main
        RUNTIME DESTINATION bin)