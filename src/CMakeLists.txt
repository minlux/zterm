cmake_minimum_required (VERSION 2.6)
project(zterm)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(zterm main.c serport.c)
