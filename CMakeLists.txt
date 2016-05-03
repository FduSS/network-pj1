cmake_minimum_required(VERSION 2.8)
project(transfer)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c11")


if(UNIX)
    set(PLATFORM_SOURCE_FILES linux.c)
endif(UNIX)

set(SOURCE_FILES main.c common.h ${PLATFORM_SOURCE_FILES})

add_executable(transfer ${SOURCE_FILES})