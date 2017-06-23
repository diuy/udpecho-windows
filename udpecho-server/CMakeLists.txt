cmake_minimum_required(VERSION 3.6)
project(udpecho_server)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
add_executable(udpecho_server ${SOURCE_FILES})