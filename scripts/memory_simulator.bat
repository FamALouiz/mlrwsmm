@echo off
echo Running Memory Management Simulator...
echo.

REM Default memory size: 1MB (1048576 bytes), Page size: 4KB (4096 bytes)
REM Optional parameters: memory_size page_size
REM Example: memory_simulator.bat 2097152 8192 - for 2MB memory with 8KB pages

cd %~dp0\..
set MEM_SIZE=1048576
set PAGE_SIZE=4096

if not "%~1"=="" set MEM_SIZE=%~1
if not "%~2"=="" set PAGE_SIZE=%~2

build\bin\memory_simulator.exe %MEM_SIZE% %PAGE_SIZE%
