cmake_minimum_required(VERSION 2.5)
project(transfer)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(LINK_LIBS )

if(APPLE)
    set(PLATFORM_SOURCE_FILES src/platform/osx.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOSX")
elseif(UNIX)
    set(PLATFORM_SOURCE_FILES src/platform/linux.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLINUX -D_GNU_SOURCE")
    set(LINK_LIBS m)
endif(APPLE)

file(GLOB SOURCE_FILES "src/*.c")

add_executable(transfer ${SOURCE_FILES} ${PLATFORM_SOURCE_FILES})
target_link_libraries(transfer ${LINK_LIBS})
