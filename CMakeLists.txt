cmake_minimum_required(VERSION 2.5)
project(transfer)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -D_GNU_SOURCE")
set(LINK_LIBS )

if(APPLE)
    set(PLATFORM_SOURCE_FILES src/platform/osx.c src/platform/bsd_linux.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOSX")
elseif (WIN32)
    set(PLATFORM_SOURCE_FILES src/platform/win.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWINDOWS")
    set(LINK_LIBS ws2_32)
elseif(UNIX)
    set(PLATFORM_SOURCE_FILES src/platform/linux.c src/platform/bsd_linux.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLINUX")
    set(LINK_LIBS m)
endif(APPLE)

file(GLOB SOURCE_FILES "src/*.c")

add_executable(transfer ${SOURCE_FILES} ${PLATFORM_SOURCE_FILES})
target_link_libraries(transfer ${LINK_LIBS})
