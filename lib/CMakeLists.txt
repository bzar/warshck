add_subdirectory(libwebsockets)


include_directories(
  libsocketio/include 
)

file(GLOB LIBSOCKETIO_SOURCES libsocketio/src/*.c)
add_library(libsocketio ${LIBSOCKETIO_SOURCES})
target_link_libraries(libsocketio uriparser)


file(GLOB JSON_SOURCES json/json.c)
add_library(json ${JSON_SOURCES})
