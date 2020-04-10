cmake_minimum_required (VERSION 2.6)
project(zterm)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(src)

add_executable(zterm
  src/main.c
  src/serport.c
)
