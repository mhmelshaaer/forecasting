cmake_minimum_required(VERSION 3.16)

project(test LANGUAGES CXX)

set(target ${PROJECT_NAME})

add_executable(${target} main.cpp test.h test.cpp)
target_link_libraries(${target} PRIVATE libnet)
