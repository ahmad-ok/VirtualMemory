cmake_minimum_required(VERSION 3.17)
project(os_ex4)

set(CMAKE_CXX_STANDARD 11)

add_executable(os_ex4 PhysicalMemory.cpp PhysicalMemory.h MemoryConstants.h VirtualMemory.h VirtualMemory.cpp SimpleTest.cpp)