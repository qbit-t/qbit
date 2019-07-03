cmake_minimum_required(VERSION 2.8)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-O3")  ## Optimize
    set(CMAKE_C_FLAGS "-O3")  ## Optimize
endif()

set (CMAKE_SHARED_LINKER_FLAGS "-Wl,-init,_jm_initialize")

set(SOURCES
    ./jm_allocator.c
    ./jm_backend.c
    ./jm_mutex.c
    ./jm_utils.c)

include_directories("./include/")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_definitions(-D__STDC_FORMAT_MACROS)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Weffc++ -Wswitch-default -g")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Weffc++ -Wswitch-default -Wfloat-equal -Wimplicit-fallthrough -Weverything -g")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
endif()

add_library(jm SHARED ${SOURCES})
target_link_libraries(jm pthread dl)

