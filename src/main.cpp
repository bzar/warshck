#include "libwebsockets.h"
#include "gamenodepp.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

#include "game.h"
#include "loggerview.h"
#include "glhckview.h"
#include "input.h"

json::Value jsonPosition(wars::Input::Position position)
{
  return json::Value::object({{"x", position.x}, {"y", position.y}});
}

json::Value jsonPath(wars::Input::Path path)
{
  json::Value result = json::Value::array();
  for(wars::Input::Position const& pos : path)
  {
    result.append(jsonPosition(pos));
  }
  return result;
}

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

  //lws_set_log_level(LLL_NOTICE | LLL_LATENCY | LLL_EXT | LLL_DEBUG | LLL_INFO | LLL_PARSER | LLL_HEADER | LLL_CLIENT | LLL_WARN | LLL_ERR | LLL_COUNT, nullptr);
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
    std::cout << "Disconnected, exiting" << std::endl;
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

  wars::Input input;

  auto buildSub = input.events.build.on([&gn](wars::Input::Build const& event) {
    json::Value params = {event.gameId, event.type, jsonPosition(event.position)};
    Promise<bool> result = event.result;
    std::cout << "Sending build command with parameters " << params.toString() << std::endl;
    gn.call("build", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto moveWaitSub = input.events.moveWait.on([&gn](wars::Input::MoveWait const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId, event.unitId, jsonPosition(event.destination), jsonPath(event.path)};
    std::cout << "Sending moveAndWait command with parameters " << params.toString() << std::endl;
    gn.call("moveAndWait", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto moveAttackSub = input.events.moveAttack.on([&gn](wars::Input::MoveAttack const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId, event.unitId, jsonPosition(event.destination), jsonPath(event.path), event.targetId};
    gn.call("moveAndAttack", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto moveDeploySub = input.events.moveDeploy.on([&gn](wars::Input::MoveDeploy const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId, event.unitId, jsonPosition(event.destination), jsonPath(event.path)};
    gn.call("moveAndDeploy", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto moveCaptureSub = input.events.moveCapture.on([&gn](wars::Input::MoveCapture const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId, event.unitId, jsonPosition(event.destination), jsonPath(event.path)};
    gn.call("moveAndCapture", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto undeploySub = input.events.undeploy.on([&gn](wars::Input::Undeploy const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId, event.unitId};
    gn.call("undeploy", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto moveLoadSub = input.events.moveLoad.on([&gn](wars::Input::MoveLoad const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId, event.unitId, event.carrierId, jsonPath(event.path)};
    gn.call("moveAndLoadInto", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto moveUnloadSub = input.events.moveUnload.on([&gn](wars::Input::MoveUnload const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId, event.unitId, jsonPosition(event.destination), jsonPath(event.path), event.carriedId, jsonPosition(event.unloadDestination)};
    gn.call("moveAndUnload", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto endTurnSub = input.events.endTurn.on([&gn](wars::Input::EndTurn const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId};
    gn.call("endTurn", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto surrenderSub = input.events.surrender.on([&gn](wars::Input::Surrender const& event) {
    Promise<bool> result = event.result;
    json::Value params = {event.gameId};
    gn.call("surrender", params).then<void>([result](json::Value const& v) mutable {
      result.fulfill(v.get("success").booleanValue());
    });
  });

  auto fundsSub = input.events.funds.on([&gn](wars::Input::Funds const& event) {
    Promise<int> result = event.result;
    json::Value params = {event.gameId};
    gn.call("myFunds", params).then<void>([result](json::Value const& v) mutable {
      if(v.get("success").booleanValue())
      {
        result.fulfill(v.get("funds").longValue());
      }
      else
      {
        result.fulfill(0);
      }
    });
  });

  wars::GlhckView::init(argc, argv);
  wars::GlhckView view(&input);
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

/* vim: set ts=2 sw=2 tw=0 :*/

