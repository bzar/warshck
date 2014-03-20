CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(warshck)

find_package(CURL REQUIRED)

add_subdirectory(lib)
include_directories(
  ${CURL_INCLUDE_DIR}
  ${LIBWEBSOCKETS_INCLUDE_DIRS}
  lib/libsocketio/include
  lib/json/
)

file(GLOB SOURCES src/*.cpp src/*.c)
list(APPEND CMAKE_CXX_FLAGS -std=c++11)
add_executable(warshck ${SOURCES})
target_link_libraries(warshck libsocketio websockets json ${CURL_LIBRARIES})

