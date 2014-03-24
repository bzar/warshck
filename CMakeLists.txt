CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(warshck)

find_package(CURL REQUIRED)

set(GLHCK_BUILD_EXAMPLES OFF CACHE BOOL "Skip GLHCK examples")
SET(GLFW_BUILD_EXAMPLES 0 CACHE BOOL "Don't build examples for GLFW")
SET(GLFW_BUILD_TESTS 0 CACHE BOOL "Don't build tests for GLFW")

add_subdirectory(lib)

include_directories(
  ${CURL_INCLUDE_DIR}
  ${LIBWEBSOCKETS_INCLUDE_DIRS}
  ${GLFW_SOURCE_DIR}/include
  ${glhck_SOURCE_DIR}/include
  ${kazmath_SOURCE_DIR}/src
  lib/libsocketio/include
  lib/json/
)

file(GLOB SOURCES src/*.cpp src/*.c)
list(APPEND CMAKE_CXX_FLAGS -std=c++11)
add_executable(warshck ${SOURCES})
target_link_libraries(warshck glfw glhck libsocketio websockets json ${CURL_LIBRARIES} ${GLFW_LIBRARIES})

install(TARGETS warshck DESTINATION .)
install(DIRECTORY assets/ DESTINATION .)
