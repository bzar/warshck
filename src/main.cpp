#include "libwebsockets.h"
#include "gamenodepp.h"
#include <iostream>
#include <cstdlib>


int main(int argc, char** argv)
{
  //lws_set_log_level(LLL_DEBUG, nullptr);
  bool running = true;
  Gamenode gn;

  gn.onConnect([&gn]() {
    std::cout << "Connected, logging in" << std::endl;
    JSONValue credentials = JSONValue::object();
    credentials.set("username", JSONValue::string("bzar"));
    credentials.set("password", JSONValue::string("bzar"));
    gn.call("newSession", credentials, [&gn](JSONValue const& response) {
      std::cout << "Got response to login: " << response.toString() << std::endl;
      gn.disconnect();
    });
  });

  gn.onDisconnect([&running]() {
    running = false;
  });

  if(!gn.connect("localhost", 8888, "/", "localhost", "localhost"))
  {
    std::cerr << "Error creating gamenode connection" << std::endl;
    return EXIT_FAILURE;
  }

  while(running)
  {
    if(!gn.handle())
    {
      std::cerr << "Gamenode connection lost" << std::endl;
      break;
    }
  }

  return EXIT_SUCCESS;
}
