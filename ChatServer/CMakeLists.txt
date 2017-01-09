cmake_minimum_required(VERSION 3.7)
project(ChatServer)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(SOURCE_FILES main.cpp helperheaders.h sockhelpers.h sockhelpers.cpp Channel.cpp Channel.h Server.cpp Server.h)
add_executable(ChatServer ${SOURCE_FILES})