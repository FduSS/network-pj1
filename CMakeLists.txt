cmake_minimum_required(VERSION 2.8)
project(transfer)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c11")

set(SOURCE_FILES main.c common.h)
add_executable(transfer ${SOURCE_FILES})