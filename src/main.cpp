#include "libwebsockets.h"
#include "gamenodepp.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

#include "game.h"
#include "loggerview.h"
#include "glhckview.h"

int main(int argc, char** argv)
{
  if(argc < 2)
  {
    std::cerr << "Usage: warshck <server> <port> <gameId> <username> <password>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string const server = argv[1];
  std::string const port = argv[2];
  std::string const gameId = argv[3];
  std::string const user = argv[4];
  std::string const pass = argv[5];

  lws_set_log_level(LLL_DEBUG | LLL_INFO | LLL_PARSER | LLL_HEADER | LLL_CLIENT, nullptr);
  bool running = true;
  Gamenode gn;
  wars::Game game;

  auto connectedSub = gn.connected().on([&gn, &gameId, &user, &pass, &game]() {
    std::cout << "Connected, logging in" << std::endl;
    JSONValue credentials = JSONValue::object();
    credentials.set("username", JSONValue::string(user));
    credentials.set("password", JSONValue::string(pass));

    gn.call("newSession", credentials).then<JSONValue>([&gn, &gameId](JSONValue const& response) {
      std::cout << "Got response to login: " << response.toString() << std::endl;
      return gn.call("subscribeGame", JSONValue::string(gameId));
    }).then<JSONValue>([&gn, &gameId](JSONValue const& response) {
      std::cout << "Subscribed to game" << std::endl;
      return gn.call("gameRules", JSONValue::string(gameId));
    }).then<JSONValue>([&gn, &gameId, &game](JSONValue const& response) {
      std::cout << "Got game rules" << std::endl;
      game.setRulesFromJSON(response);
      return gn.call("gameData", JSONValue::string(gameId));
    }).then<void>([&gn, &game](JSONValue const& response) {
      std::cout << "Got game data" << std::endl;
      game.setGameDataFromJSON(response);
    });
  });

  auto disconnectedSub = gn.disconnected().on([&running]() {
    running = false;
  });

  //Skeleton::playerJoined = (gameId, playerNumber, playerName, isMe) ->
  gn.onVoidMethod("playerJoined", [](JSONValue const& params) {
    std::cout << "playerJoined " << params.toString() << std::endl;
  });
  //Skeleton::playerLeft = (gameId, playerNumber) ->
  gn.onVoidMethod("playerLeft", [](JSONValue const& params) {
    std::cout << "playerLeft" << params.toString() << std::endl;
  });

  //Skeleton::playerTeamChanged = (gameId, playerNumber, teamNumber, playerName, isMe) ->
  gn.onVoidMethod("playerTeamChanged", [](JSONValue const& params) {
    std::cout << "playerTeamChanged" << params.toString() << std::endl;
  });

  //Skeleton::bannedUnits = (unitTypes) ->
  gn.onVoidMethod("bannedUnits", [](JSONValue const& params) {
    std::cout << "bannedUnits" << params.toString() << std::endl;
  });

  //Skeleton::gameStarted = (gameId) ->
  gn.onVoidMethod("gameStarted", [](JSONValue const& params) {
    std::cout << "gameStarted" << params.toString() << std::endl;
  });

  //Skeleton::gameFinished = (gameId) ->
  gn.onVoidMethod("gameFinished", [](JSONValue const& params) {
    std::cout << "gameFinished" << params.toString() << std::endl;
  });

  //Skeleton::gameTurnChange = (gameId, newTurn, newRound, turnRemaining) ->
  gn.onVoidMethod("gameTurnChange", [](JSONValue const& params) {
    std::cout << "gameTurnChange" << params.toString() << std::endl;
  });

  //Skeleton::gameEvents = (gameId, events) ->
  gn.onVoidMethod("gameEvents", [&gn, &game](JSONValue const& params) {
    JSONValue events = params.at(1);
    game.processEventsFromJSON(events);
  });

  //Skeleton::chatMessage = (messageInfo) ->
  gn.onVoidMethod("chatMessage", [](JSONValue const& params) {
    std::cout << "chatMessage" << params.toString() << std::endl;
  });

  //Skeleton::addInvite = (gameId) ->
  gn.onVoidMethod("addInvite", [](JSONValue const& params) {
    std::cout << "addInvite" << params.toString() << std::endl;
  });

  //Skeleton::removeInvite = (gameId) ->
  gn.onVoidMethod("remoteInvite", [](JSONValue const& params) {
    std::cout << "remoteInvite " << params.toString() << std::endl;
  });

  int portInt;
  std::istringstream(port) >> portInt;
  if(!gn.connect(server, portInt, "/", server, server))
  {
    std::cerr << "Error creating gamenode connection" << std::endl;
    return EXIT_FAILURE;
  }

  wars::LoggerView logger;
  logger.setGame(&game);

  wars::GlhckView::init(argc, argv);
  wars::GlhckView view(&gn);
  view.setGame(&game);

  while(running)
  {
    usleep(1000);
    if(!gn.handle())
    {
      std::cerr << "Gamenode connection lost" << std::endl;
      break;
    }

    if(!logger.handle() || !view.handle())
    {
      break;
    }
  }

  wars::GlhckView::term();

  return EXIT_SUCCESS;
}
