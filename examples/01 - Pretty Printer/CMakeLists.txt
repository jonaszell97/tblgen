
cmake_minimum_required(VERSION 3.10)
project(TblGenExample01)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -Wall -Wl,-undefined -Wl,dynamic_lookup")

include_directories("$ENV{TBLGEN_PATH}/include")

add_library(example01 SHARED Backend.cpp)

