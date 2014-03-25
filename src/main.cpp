#include "libwebsockets.h"
#include "gamenodepp.h"
#include <iostream>
#include <cstdlib>

#include "game.h"
#include "loggerview.h"
#include "glhckview.h"

int main(int argc, char** argv)
{
  if(argc < 2)
  {
    std::cerr << "Usage: warshck <gameId>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string const gameId = argv[1];
  std::string const user = argv[2];
  std::string const pass = argv[3];

  //lws_set_log_level(LLL_DEBUG, nullptr);
  bool running = true;
  Gamenode gn;
  wars::Game game;

  gn.connected().on<void>([&gn, &gameId, &user, &pass, &game]() {
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

  gn.disconnected().on<void>([&running]() {
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
  gn.onMethod("gameEvents", [&gn, &game](JSONValue const& params) {
    JSONValue events = params.at(1);
    game.processEventsFromJSON(events);
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
