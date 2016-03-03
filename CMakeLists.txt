cmake_minimum_required(VERSION 3.3)
project(sortdir)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    constants.h
    sortdir.c)

add_executable(sortdir ${SOURCE_FILES})