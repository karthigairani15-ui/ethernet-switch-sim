cmake_minimum_required(VERSION 3.15)
project(ethernet_switch_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(simulator src/main.cpp)
