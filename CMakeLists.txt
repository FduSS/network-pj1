project(transfer)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

if(LINUX)
    set(PLATFORM_SOURCE_FILES linux.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLINUX")
elseif(APPLE)
    set(PLATFORM_SOURCE_FILES osx.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOSX")
endif(LINUX)

set(SOURCE_FILES main.c common.h ${PLATFORM_SOURCE_FILES} task.c task.h config.c config.c config.h)

add_executable(transfer ${SOURCE_FILES})