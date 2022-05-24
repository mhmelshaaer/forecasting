cmake_minimum_required(VERSION 3.16)

project(client LANGUAGES CXX)

set(target ${PROJECT_NAME})

if(NOT SERVER_HOST_NAME)

  set(SERVER_HOST_NAME "server")

endif()

add_subdirectory(lib/net-client-server)
add_subdirectory(test)

add_executable(${target} main.cpp)
target_compile_definitions(${target} PUBLIC SERVER_HOST_NAME=${SERVER_HOST_NAME})
target_link_libraries(${target} PRIVATE libnet)
