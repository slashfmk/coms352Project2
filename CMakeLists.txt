cmake_minimum_required(VERSION 3.23)
project(proj2 C)

set(CMAKE_C_STANDARD 99)

add_executable(proj2 journal.c filesystem.c block_service.c)
