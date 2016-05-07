cmake_minimum_required(VERSION 2.5)
project(transfer)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

if(LINUX)
    set(PLATFORM_SOURCE_FILES src/platform/linux.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLINUX")
elseif(APPLE)
    set(PLATFORM_SOURCE_FILES src/platform/osx.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOSX")
endif(LINUX)

file(GLOB SOURCE_FILES "src/*.c")

add_executable(transfer ${SOURCE_FILES} ${PLATFORM_SOURCE_FILES})