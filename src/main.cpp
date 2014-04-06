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

  lws_set_log_level(LLL_DEBUG | LLL_INFO | LLL_PARSER | LLL_HEADER | LLL_CLIENT | LLL_WARN | LLL_ERR | LLL_COUNT, nullptr);
  bool running = true;
  Gamenode gn;
  wars::Game game;

  auto connectedSub = gn.connected().on([&gn, &gameId, &user, &pass, &game]() {
    std::cout << "Connected, logging in" << std::endl;
    json::Value credentials = json::Value::object({
                                                    {"username", user},
                                                    {"password", pass}
                                                  });

    gn.call("newSession", credentials).then<json::Value>([&gn, &gameId](json::Value const& response) {
      std::cout << "Got response to login: " << response.toString() << std::endl;
      return gn.call("subscribeGame", json::Value(gameId));
    }).then<json::Value>([&gn, &gameId](json::Value const& response) {
      std::cout << "Subscribed to game" << std::endl;
      return gn.call("gameRules", json::Value(gameId));
    }).then<json::Value>([&gn, &gameId, &game](json::Value const& response) {
      std::cout << "Got game rules" << std::endl;
      game.setRulesFromJSON(response);
      return gn.call("gameData", json::Value(gameId));
    }).then<void>([&gn, &game](json::Value const& response) {
      std::cout << "Got game data" << std::endl;
      game.setGameDataFromJSON(response);
    });
  });

  auto disconnectedSub = gn.disconnected().on([&running]() {
    running = false;
  });

  //Skeleton::playerJoined = (gameId, playerNumber, playerName, isMe) ->
  gn.onVoidMethod("playerJoined", [](json::Value const& params) {
    std::cout << "playerJoined " << params.toString() << std::endl;
  });
  //Skeleton::playerLeft = (gameId, playerNumber) ->
  gn.onVoidMethod("playerLeft", [](json::Value const& params) {
    std::cout << "playerLeft" << params.toString() << std::endl;
  });

  //Skeleton::playerTeamChanged = (gameId, playerNumber, teamNumber, playerName, isMe) ->
  gn.onVoidMethod("playerTeamChanged", [](json::Value const& params) {
    std::cout << "playerTeamChanged" << params.toString() << std::endl;
  });

  //Skeleton::bannedUnits = (unitTypes) ->
  gn.onVoidMethod("bannedUnits", [](json::Value const& params) {
    std::cout << "bannedUnits" << params.toString() << std::endl;
  });

  //Skeleton::gameStarted = (gameId) ->
  gn.onVoidMethod("gameStarted", [](json::Value const& params) {
    std::cout << "gameStarted" << params.toString() << std::endl;
  });

  //Skeleton::gameFinished = (gameId) ->
  gn.onVoidMethod("gameFinished", [](json::Value const& params) {
    std::cout << "gameFinished" << params.toString() << std::endl;
  });

  //Skeleton::gameTurnChange = (gameId, newTurn, newRound, turnRemaining) ->
  gn.onVoidMethod("gameTurnChange", [](json::Value const& params) {
    std::cout << "gameTurnChange" << params.toString() << std::endl;
  });

  //Skeleton::gameEvents = (gameId, events) ->
  gn.onVoidMethod("gameEvents", [&gn, &game](json::Value const& params) {
    json::Value events = params.at(1);
    game.processEventsFromJSON(events);
  });

  //Skeleton::chatMessage = (messageInfo) ->
  gn.onVoidMethod("chatMessage", [](json::Value const& params) {
    std::cout << "chatMessage" << params.toString() << std::endl;
  });

  //Skeleton::addInvite = (gameId) ->
  gn.onVoidMethod("addInvite", [](json::Value const& params) {
    std::cout << "addInvite" << params.toString() << std::endl;
  });

  //Skeleton::removeInvite = (gameId) ->
  gn.onVoidMethod("remoteInvite", [](json::Value const& params) {
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
