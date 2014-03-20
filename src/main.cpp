#include "libwebsockets.h"
#include "gamenodepp.h"
#include <iostream>
#include <cstdlib>


int main(int argc, char** argv)
{
  if(argc < 2)
  {
    std::cerr << "Usage: warshck <gameId>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string const gameId = argv[1];
  lws_set_log_level(LLL_DEBUG, nullptr);
  bool running = true;
  Gamenode gn;

  gn.onConnect([&gn, &gameId]() {
    std::cout << "Connected, logging in" << std::endl;
    JSONValue credentials = JSONValue::object();
    credentials.set("username", JSONValue::string("bzar"));
    credentials.set("password", JSONValue::string("bzar"));
    gn.call("newSession", credentials, [&gn, &gameId](JSONValue const& response) {
      std::cout << "Got response to login: " << response.toString() << std::endl;
      gn.call("subscribeGame", JSONValue::string(gameId), [&gn](JSONValue const& response) {
        std::cout << "Subscribed to game" << std::endl;
        gn.disconnect();
      });
    });
  });

  gn.onDisconnect([&running]() {
    running = false;
  });

  //Skeleton::playerJoined = (gameId, playerNumber, playerName, isMe) ->
  gn.onMethod("playerJoined", [](JSONValue const& params) {
    std::cout << "playerJoined " << params.toString() << std::endl;
    return JSONValue::null();
  });
  //Skeleton::playerLeft = (gameId, playerNumber) ->
  gn.onMethod("playerLeft", [](JSONValue const& params) {
    std::cout << "playerLeft" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::playerTeamChanged = (gameId, playerNumber, teamNumber, playerName, isMe) ->
  gn.onMethod("playerTeamChanged", [](JSONValue const& params) {
    std::cout << "playerTeamChanged" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::bannedUnits = (unitTypes) ->
  gn.onMethod("bannedUnits", [](JSONValue const& params) {
    std::cout << "bannedUnits" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::gameStarted = (gameId) ->
  gn.onMethod("gameStarted", [](JSONValue const& params) {
    std::cout << "gameStarted" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::gameFinished = (gameId) ->
  gn.onMethod("gameFinished", [](JSONValue const& params) {
    std::cout << "gameFinished" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::gameTurnChange = (gameId, newTurn, newRound, turnRemaining) ->
  gn.onMethod("gameTurnChange", [](JSONValue const& params) {
    std::cout << "gameTurnChange" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::gameEvents = (gameId, events) ->
  gn.onMethod("gameEvents", [](JSONValue const& params) {
    std::cout << "gameEvents" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::chatMessage = (messageInfo) ->
  gn.onMethod("chatMessage", [](JSONValue const& params) {
    std::cout << "chatMessage" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::addInvite = (gameId) ->
  gn.onMethod("addInvite", [](JSONValue const& params) {
    std::cout << "addInvite" << params.toString() << std::endl;
    return JSONValue::null();
  });

  //Skeleton::removeInvite = (gameId) ->
  gn.onMethod("remoteInvite", [](JSONValue const& params) {
    std::cout << "remoteInvite " << params.toString() << std::endl;
    return JSONValue::null();
  });

  if(!gn.connect("localhost", 8888, "/", "localhost", "localhost"))
  {
    std::cerr << "Error creating gamenode connection" << std::endl;
    return EXIT_FAILURE;
  }

  while(running)
  {
    usleep(1000);
    if(!gn.handle())
    {
      std::cerr << "Gamenode connection lost" << std::endl;
      break;
    }
  }

  return EXIT_SUCCESS;
}
