cmake_minimum_required(VERSION 3.16)

project(server LANGUAGES CXX)

set(target ${PROJECT_NAME})

add_executable(${target} main.cpp)
