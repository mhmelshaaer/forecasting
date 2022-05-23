set(target libnet)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_library(${target} INTERFACE)

target_compile_features(${target} INTERFACE cxx_std_17)

target_include_directories(
  ${target} INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(lib/asio-1.18.1)

target_link_libraries(${target} INTERFACE Threads::Threads asio)