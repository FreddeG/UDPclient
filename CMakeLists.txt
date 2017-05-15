cmake_minimum_required(VERSION 3.7)
project(UDPclient)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c list.c list.h Generic.c Generic.h)
FIND_PACKAGE(Threads)
add_executable(UDPclient ${SOURCE_FILES})
target_link_libraries( UDPclient ${CMAKE_THREAD_LIBS_INIT} )