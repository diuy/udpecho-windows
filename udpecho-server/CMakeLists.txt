cmake_minimum_required(VERSION 2.8)
project(udpecho_server)

set(CMAKE_CXX_STANDARD 98)

set(SOURCE_FILES main.cpp)
add_executable(udpecho_server ${SOURCE_FILES})