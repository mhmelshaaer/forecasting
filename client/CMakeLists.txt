cmake_minimum_required(VERSION 3.16)

project(client LANGUAGES CXX)

set(target ${PROJECT_NAME})

add_subdirectory(lib/net-client-server)

add_executable(${target} main.cpp)
target_link_libraries(${target} PRIVATE libnet)
