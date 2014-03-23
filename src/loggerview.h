#ifndef WARS_LOGGERVIEW_H
#define WARS_LOGGERVIEW_H

#include "view.h"
#include <iostream>
#include <chrono>
#include <iomanip>

namespace wars
{
  class LoggerView : public View
  {
  public:
    void setGame(Game* game) override
    {
      game->events().on<void>([game](wars::Game::Event const& e) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << now << ": ";

        switch(e.type)
        {
          case wars::Game::EventType::GAMEDATA:
          {
            std::cout << "Initial gamedata available" << std::endl;
            break;
          }
          case wars::Game::EventType::MOVE:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.move.unitId);
            wars::Game::Tile const& next = game->getTile(*e.move.tileId);
            wars::Game::Tile const& prev = game->getTile(unit.tileId);
            std::cout << "Unit " << unit.id << " moves from (" << prev.x << ", " << prev.y  << ") to (" << next.x << ", " << next.y << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::WAIT:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.wait.unitId);
            wars::Game::Tile const& curr = game->getTile(unit.tileId);
            std::cout << "Unit " << unit.id << " waits at (" << curr.x << ", " << curr.y  << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::ATTACK:
          {
            wars::Game::Unit const& attacker = game->getUnit(*e.attack.attackerId);
            wars::Game::Unit const& target = game->getUnit(*e.attack.targetId);
            std::cout << "Unit " << attacker.id << " attacks unit " << target.id << ", inflicts " << e.attack.damage  << " points damage" << std::endl;
            break;
          }
          case wars::Game::EventType::COUNTERATTACK:
          {
            wars::Game::Unit const& attacker = game->getUnit(*e.counterattack.attackerId);
            wars::Game::Unit const& target = game->getUnit(*e.counterattack.targetId);
            std::cout << "Unit " << attacker.id << " counterattacks unit " << target.id << ", inflicts " << e.counterattack.damage  << " points damage" << std::endl;
            break;
          }
          case wars::Game::EventType::CAPTURE:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.capture.unitId);
            wars::Game::Tile const& tile = game->getTile(*e.capture.tileId);
            std::cout << "Unit " << unit.id << " captures tile at (" << tile.x << ", " << tile.y  << "), " << e.capture.left << " capture points left" << std::endl;
            break;
          }
          case wars::Game::EventType::CAPTURED:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.captured.unitId);
            wars::Game::Tile const& tile = game->getTile(*e.captured.tileId);
            std::cout << "Unit " << unit.id << " captured tile at (" << tile.x << ", " << tile.y  << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::DEPLOY:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.deploy.unitId);
            wars::Game::Tile const& curr = game->getTile(unit.tileId);
            std::cout << "Unit " << unit.id << " deploys at (" << curr.x << ", " << curr.y  << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::UNDEPLOY:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.undeploy.unitId);
            wars::Game::Tile const& curr = game->getTile(unit.tileId);
            std::cout << "Unit " << unit.id << " undeploys at (" << curr.x << ", " << curr.y  << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::LOAD:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.load.unitId);
            wars::Game::Unit const& carrier = game->getUnit(*e.load.carrierId);
            wars::Game::Tile const& curr = game->getTile(unit.tileId);
            std::cout << "Unit " << unit.id << " loads into unit " << carrier.id << " at (" << curr.x << ", " << curr.y  << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::UNLOAD:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.unload.unitId);
            wars::Game::Unit const& carrier = game->getUnit(*e.unload.carrierId);
            wars::Game::Tile const& next = game->getTile(*e.unload.tileId);
            std::cout << "Unit " << unit.id << " unloads from unit " << carrier.id << " to (" << next.x << ", " << next.y  << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::DESTROY:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.destroy.unitId);
            wars::Game::Tile const& curr = game->getTile(unit.tileId);
            std::cout << "Unit " << unit.id << " destroyed at (" << curr.x << ", " << curr.y  << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::REPAIR:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.repair.unitId);
            wars::Game::Tile const& curr = game->getTile(unit.tileId);
            std::cout << "Unit " << unit.id << " repaired at (" << curr.x << ", " << curr.y  << "), new health = " << e.repair.newHealth << " points" << std::endl;
            break;
          }
          case wars::Game::EventType::BUILD:
          {
            wars::Game::Unit const& unit = game->getUnit(*e.build.unitId);
            wars::Game::Tile const& tile = game->getTile(*e.build.tileId);
            std::cout << "Unit " << unit.id << " built by tile at (" << tile.x << ", " << tile.y  << ")" << std::endl;
            break;
          }
          case wars::Game::EventType::REGENERATE_CAPTURE_POINTS:
          {
            wars::Game::Tile const& tile = game->getTile(*e.regenerateCapturePoints.tileId);
            std::cout << "Tile at (" << tile.x << ", " << tile.y  << ") regenerates capture points, new value = " << e.regenerateCapturePoints.newCapturePoints << std::endl;
            break;
          }
          case wars::Game::EventType::PRODUCE_FUNDS:
          {
            wars::Game::Tile const& tile = game->getTile(*e.produceFunds.tileId);
            std::cout << "Tile at (" << tile.x << ", " << tile.y  << ") produces funds" << std::endl;
            break;
          }
          case wars::Game::EventType::BEGIN_TURN:
          {
            std::cout << "Player " << e.beginTurn.playerNumber << " begins turn" << std::endl;
            break;
          }
          case wars::Game::EventType::END_TURN:
          {
            std::cout << "Player " << e.endTurn.playerNumber << " end turn" << std::endl;
            break;
          }
          case wars::Game::EventType::TURN_TIMEOUT:
          {
            std::cout << "Turn timeout for player " << e.turnTimeout.playerNumber << std::endl;
            break;
          }
          case wars::Game::EventType::FINISHED:
          {
            std::cout << "Game finished, player " << e.finished.winnerPlayerNumber << " wins" << std::endl;
            break;
          }
          case wars::Game::EventType::SURRENDER:
          {
            std::cout << "Player " << e.surrender.playerNumber << " surrenders" << std::endl;
            break;
          }
          default:
          {
            break;
          }
        }
      });
    }

    bool handle() override
    {
      return true;
    }
  };
}
#endif // WARS_LOGGERVIEW_H
