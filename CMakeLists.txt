cmake_minimum_required(VERSION 3.10)
project(logging-daemon)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -pedantic")

add_executable(logging-daemon main.c)
