#include "game.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <queue>
#include <cmath>
#include <map>

#include "jsonpp.h"

std::unordered_map<std::string, wars::Game::State> const wars::Game::STATE_NAMES = {
  {"pregame", State::PREGAME},
  {"inProgress", State::IN_PROGRESS},
  {"finished", State::FINISHED}
};

namespace
{
  template<typename T>
  T parse(json::Value const& v);

  template<typename T>
  std::unordered_map<int, T> parseAll(json::Value const& v);

  template<>
  wars::Weapon parse(json::Value const& v);
  template<>
  wars::Armor parse(json::Value const& v);
  template<>
  wars::UnitClass parse(json::Value const& v);
  template<>
  wars::TerrainFlag parse(json::Value const& v);
  template<>
  wars::TerrainType parse(json::Value const& v);
  template<>
  wars::MovementType parse(json::Value const& v);
  template<>
  wars::UnitFlag parse(json::Value const& v);
  template<>
  wars::UnitType parse(json::Value const& v);
  template<>
  wars::Rules parse(json::Value const& value);

  std::unordered_map<int, int> parseIntIntMap(json::Value const& v);
  std::unordered_map<int, int> parseIntIntMapWithNulls(json::Value const& v, int nullValue);
  std::unordered_set<int> parseIntSet(json::Value const& v);
  int parseIntOrNull(json::Value const& v, int nullValue);
  std::string parseStringOrNull(json::Value const& v, std::string const& nullValue);
  wars::Game::Path parsePath(json::Value const& v);
}
wars::Game::Game(): gameId(), authorId(),  name(), mapId(),
  state(State::PREGAME), turnStart(0), turnNumber(0), roundNumber(0), inTurnNumber(0),
  publicGame(false), turnLength(0), bannedUnits(0),
  rules(), tiles(), units(),  players(), eventStream()
{

}

wars::Game::~Game()
{

}

Stream<wars::Game::Event> wars::Game::events()
{
  return eventStream;
}

void wars::Game::setRulesFromJSON(const json::Value& value)
{
  rules = parse<Rules>(value);
}

void wars::Game::setGameDataFromJSON(const json::Value& value)
{
  json::Value game = value.get("game");
  gameId = game.get("gameId").stringValue();
  authorId = game.get("authorId").stringValue();
  name = game.get("name").stringValue();
  mapId = game.get("mapId").stringValue();
  state = STATE_NAMES.at(game.get("state").stringValue());
  turnStart = game.get("turnStart").longValue();
  turnNumber = game.get("turnNumber").longValue();
  roundNumber = game.get("roundNumber").longValue();
  inTurnNumber = game.get("inTurnNumber").longValue();

  json::Value settings = game.get("settings");
  publicGame = settings.get("public").booleanValue();
  turnLength = parseIntOrNull(settings.get("turnLength"), -1);
  bannedUnits = parseIntSet(settings.get("bannedUnits"));

  json::Value tileArray = game.get("tiles");
  unsigned int numTiles = tileArray.size();
  for(unsigned int i = 0; i < numTiles; ++i)
  {
    json::Value tile = tileArray.at(i);
    updateTileFromJSON(tile);
  }

  json::Value playerArray = game.get("players");
  unsigned int numPlayers = playerArray.size();
  for(unsigned int i = 0; i < numPlayers; ++i)
  {
    json::Value player = playerArray.at(i);
    updatePlayerFromJSON(player);
  }

  Event event;
  event.type = EventType::GAMEDATA;
  eventStream.push(event);
}

void wars::Game::processEventFromJSON(const json::Value& value)
{
  json::Value content = value.get("content");
  std::string action = content.get("action").stringValue();

  if(action == "move")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    std::string tileId = content.get("tile").get("tileId").stringValue();
    Path path = parsePath(content.get("path"));
    moveUnit(unitId, tileId, path);
  }
  else if(action == "wait")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    waitUnit(unitId);
  }
  else if(action == "attack")
  {
    std::string attackerId = content.get("attacker").get("unitId").stringValue();
    std::string targetId = content.get("target").get("unitId").stringValue();
    int damage = content.get("damage").longValue();
    attackUnit(attackerId, targetId, damage);
  }
  else if(action == "counterattack")
  {
    std::string attackerId = content.get("attacker").get("unitId").stringValue();
    std::string targetId = content.get("target").get("unitId").stringValue();
    int damage = -1;
    if(content.get("damage").type() == json::Value::Type::NUMBER)
    {
      damage = content.get("damage").longValue();
    }
    counterattackUnit(attackerId, targetId, damage);
  }
  else if(action == "capture")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    std::string tileId = content.get("tile").get("tileId").stringValue();
    int left = content.get("left").longValue();
    captureTile(unitId, tileId, left);
  }
  else if(action == "captured")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    std::string tileId = content.get("tile").get("tileId").stringValue();
    capturedTile(unitId, tileId);
  }
  else if(action == "deploy")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    deployUnit(unitId);
  }
  else if(action == "undeploy")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    undeployUnit(unitId);
  }
  else if(action == "load")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    std::string carrierId = content.get("carrier").get("unitId").stringValue();
    loadUnit(unitId, carrierId);
  }
  else if(action == "unload")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    std::string carrierId = content.get("carrier").get("unitId").stringValue();
    std::string tileId = content.get("tile").get("tileId").stringValue();
    unloadUnit(unitId, carrierId, tileId);
  }
  else if(action == "destroyed")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    destroyUnit(unitId);
  }
  else if(action == "repair")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    int newHealth = content.get("newHealth").longValue();
    repairUnit(unitId, newHealth);
  }
  else if(action == "build")
  {
    std::string tileId = content.get("tile").get("tileId").stringValue();
    std::string unitId = updateUnitFromJSON(content.get("unit"));
    units[unitId].tileId = tileId;
    buildUnit(tileId, unitId);
  }
  else if(action == "regenerateCapturePoints")
  {
    std::string tileId = content.get("tile").get("tileId").stringValue();
    int newCapturePoints = content.get("newCapturePoints").longValue();
    regenerateCapturePointsTile(tileId, newCapturePoints);
  }
  else if(action == "produceFunds")
  {
    std::string tileId = content.get("tile").get("tileId").stringValue();
    produceFundsTile(tileId);
  }
  else if(action == "beginTurn")
  {
    int playerNumber = content.get("player").longValue();
    beginTurn(playerNumber);
  }
  else if(action == "endTurn")
  {
    int playerNumber = content.get("player").longValue();
    endTurn(playerNumber);
  }
  else if(action == "turnTimeout")
  {
    int playerNumber = content.get("player").longValue();
    turnTimeout(playerNumber);
  }
  else if(action == "finished")
  {
    int winnerPlayerNumber = content.get("winner").longValue();
    finished(winnerPlayerNumber);
  }
  else if(action == "surrender")
  {
    int playerNumber = content.get("player").longValue();
    surrender(playerNumber);
  }
  else
  {
    std::cerr << "Unknown event action: " << action << std::endl;
  }
}

void wars::Game::processEventsFromJSON(const json::Value& value)
{
  unsigned int numEvents = value.size();
  for(int i = 0; i < numEvents; ++i)
  {
    json::Value event = value.at(i);
    processEventFromJSON(event);
  }
}

void wars::Game::moveUnit(std::string const& unitId, std::string const& tileId, Path const& path)
{
  Event event;
  event.type = EventType::MOVE;
  event.move.unitId = &unitId;
  event.move.tileId = &tileId;
  event.move.path = &path;
  eventStream.push(event);

  Unit& unit = units.at(unitId);
  Tile& tile = tiles.at(tileId);
  tiles.at(unit.tileId).unitId.clear();
  if(tile.unitId.empty())
    tile.unitId = unitId;
  unit.tileId = tileId;
}

void wars::Game::waitUnit(std::string const& unitId)
{
  Event event;
  event.type = EventType::WAIT;
  event.wait.unitId = &unitId;
  eventStream.push(event);

  units.at(unitId).moved = true;
}

void wars::Game::attackUnit(std::string const& attackerId, std::string const& targetId, int damage)
{
  Event event;
  event.type = EventType::ATTACK;
  event.attack.attackerId = &attackerId;
  event.attack.targetId = &targetId;
  event.attack.damage = damage;
  eventStream.push(event);

units.at(attackerId).moved = true;
units.at(targetId).health -= damage;
}

void wars::Game::counterattackUnit(std::string const& attackerId, std::string const& targetId, int damage)
{
  Event event;
  event.type = EventType::COUNTERATTACK;
  event.counterattack.attackerId = &attackerId;
  event.counterattack.targetId = &targetId;
  event.counterattack.damage = damage;
  eventStream.push(event);

  units.at(targetId).health -= damage;
}

void wars::Game::captureTile(std::string const& unitId, std::string const& tileId, int left)
{
  Event event;
  event.type = EventType::CAPTURE;
  event.capture.unitId = &unitId;
  event.capture.tileId = &tileId;
  event.capture.left = left;
  eventStream.push(event);

  units.at(unitId).moved = true;
  Tile& tile = tiles.at(tileId);
  tile.capturePoints = left;
  tile.beingCaptured = true;
}

void wars::Game::capturedTile(std::string const& unitId, std::string const& tileId)
{
  Event event;
  event.type = EventType::CAPTURED;
  event.captured.unitId = &unitId;
  event.captured.tileId = &tileId;
  eventStream.push(event);

  Tile& tile = tiles.at(tileId);
  tile.capturePoints = 1;
  tile.beingCaptured = false;
  tile.owner = units.at(unitId).owner;
}

void wars::Game::deployUnit(std::string const& unitId)
{
  Event event;
  event.type = EventType::DEPLOY;
  event.deploy.unitId = &unitId;
  eventStream.push(event);

  Unit& unit = units.at(unitId);
  unit.moved = true;
  unit.deployed = true;
}

void wars::Game::undeployUnit(std::string const& unitId)
{
  Event event;
  event.type = EventType::UNDEPLOY;
  event.undeploy.unitId = &unitId;
  eventStream.push(event);

  Unit& unit = units.at(unitId);
  unit.moved = true;
  unit.deployed = false;
}

void wars::Game::loadUnit(std::string const& unitId, std::string const& carrierId)
{
  Event event;
  event.type = EventType::LOAD;
  event.load.unitId = &unitId;
  event.load.carrierId = &carrierId;
  eventStream.push(event);

  Unit& unit = units.at(unitId);
  unit.tileId.clear();
  unit.carriedBy = carrierId;
  unit.moved = true;
  Unit& carrier = units.at(carrierId);
  carrier.carriedUnits.push_back(unitId);
}

void wars::Game::unloadUnit(std::string const& unitId, std::string const& carrierId, std::string const& tileId)
{
  Event event;
  event.type = EventType::UNLOAD;
  event.unload.unitId = &unitId;
  event.unload.carrierId = &carrierId;
  event.unload.tileId = &tileId;
  eventStream.push(event);

  Unit& unit = units.at(unitId);
  unit.tileId = tileId;
  unit.moved = true;
  tiles.at(tileId).unitId = unitId;
  Unit& carrier = units[carrierId];
  carrier.moved = true;
  std::remove(carrier.carriedUnits.begin(), carrier.carriedUnits.end(), unitId);
}

void wars::Game::destroyUnit(std::string const& unitId)
{
  Event event;
  event.type = EventType::DESTROY;
  event.destroy.unitId = &unitId;
  eventStream.push(event);

  Unit unit = units.at(unitId);
  if(!unit.tileId.empty())
    tiles.at(unit.tileId).unitId.erase();

  for(std::string const& carriedUnitId : unit.carriedUnits)
  {
    destroyUnit(carriedUnitId);
  }

  units.erase(unitId);
}

void wars::Game::repairUnit(std::string const& unitId, int newHealth)
{
  Event event;
  event.type = EventType::REPAIR;
  event.repair.unitId = &unitId;
  event.repair.newHealth = newHealth;
  eventStream.push(event);

  units.at(unitId).health = newHealth;
}

void wars::Game::buildUnit(std::string const& tileId, std::string const& unitId)
{
  Event event;
  event.type = EventType::BUILD;
  event.build.tileId = &tileId;
  event.build.unitId = &unitId;
  eventStream.push(event);

  tiles.at(tileId).unitId = unitId;
  units.at(unitId).moved = true;
}

void wars::Game::regenerateCapturePointsTile(std::string const& tileId, int newCapturePoints)
{
  Event event;
  event.type = EventType::REGENERATE_CAPTURE_POINTS;
  event.regenerateCapturePoints.tileId = &tileId;
  event.regenerateCapturePoints.newCapturePoints = newCapturePoints;
  eventStream.push(event);

  Tile& tile = tiles.at(tileId);
  tile.capturePoints = newCapturePoints;
  tile.beingCaptured = false;

}

void wars::Game::produceFundsTile(std::string const& tileId)
{
  Event event;
  event.type = EventType::PRODUCE_FUNDS;
  event.produceFunds.tileId = &tileId;
  eventStream.push(event);
}

void wars::Game::beginTurn(int playerNumber)
{
  Event event;
  event.type = EventType::BEGIN_TURN;
  event.beginTurn.playerNumber = playerNumber;
  eventStream.push(event);

  inTurnNumber = playerNumber;
}

void wars::Game::endTurn(int playerNumber)
{
  Event event;
  event.type = EventType::END_TURN;
  event.endTurn.playerNumber = playerNumber;
  eventStream.push(event);

  for(auto& item : units)
  {
    Unit& unit = item.second;
    unit.moved = false;
  }
}

void wars::Game::turnTimeout(int playerNumber)
{
  Event event;
  event.type = EventType::TURN_TIMEOUT;
  event.turnTimeout.playerNumber = playerNumber;
  eventStream.push(event);
}

void wars::Game::finished(int winnerPlayerNumber)
{
  Event event;
  event.type = EventType::FINISHED;
  event.finished.winnerPlayerNumber = winnerPlayerNumber;
  eventStream.push(event);

  state = State::FINISHED;
}

void wars::Game::surrender(int playerNumber)
{
  Event event;
  event.type = EventType::SURRENDER;
  event.surrender.playerNumber = playerNumber;
  eventStream.push(event);

  std::vector<std::string> unitsToDestroy;
  for(auto& item : units)
  {
    Unit& unit = item.second;
    if(unit.owner == playerNumber)
    {
      unitsToDestroy.push_back(unit.id);
    }
  }

  for(std::string const& unitId : unitsToDestroy)
  {
    destroyUnit(unitId);
  }

  for(auto& item : tiles)
  {
    Tile& tile = item.second;
    if(tile.owner == playerNumber)
    {
      tile.owner = NEUTRAL_PLAYER_NUMBER;
    }
  }
}

wars::Game::Tile const & wars::Game::getTile(const std::string& tileId) const
{
  return tiles.at(tileId);
}

wars::Game::Unit const& wars::Game::getUnit(const std::string& unitId) const
{
  return units.at(unitId);
}

wars::Game::Player const& wars::Game::getPlayer(int playerNumber) const
{
  return players.at(playerNumber);
}

const std::unordered_map<std::string, wars::Game::Tile>& wars::Game::getTiles() const
{
  return tiles;
}

const std::unordered_map<std::string, wars::Game::Unit>& wars::Game::getUnits() const
{
  return units;
}

const std::unordered_map<int, wars::Game::Player>& wars::Game::getPlayers() const
{
  return players;
}

const wars::Rules& wars::Game::getRules() const
{
  return rules;
}

wars::Game::Player const& wars::Game::getInTurn()
{
  return players.at(inTurnNumber) ;
}

const wars::Game::Tile* wars::Game::getTileAt(int x, int y) const
{
  for(auto const& item : tiles)
  {
    if(item.second.x == x && item.second.y == y)
    {
      return &item.second;
    }
  }

  return nullptr;
}

const std::string& wars::Game::getGameId() const
{
  return gameId;
}

int wars::Game::calculateDistance(const wars::Game::Coordinates& a, const wars::Game::Coordinates& b) const
{
  int distance = 0;
  Coordinates aa = a;
  Coordinates bb = b;

  if(a.x < b.x && a.y > b.y)
  {
    int diagonal = std::min(b.x - a.x, a.y - b.y);
    aa.x += diagonal;
    aa.y -= diagonal;
    distance += diagonal;
  }
  else if(a.x > b.x && a.y < b.y)
  {
    int diagonal = std::min(a.x - b.x, b.y - a.y);
    bb.x += diagonal;
    bb.y -= diagonal;
    distance += diagonal;
  }
  distance += std::abs(bb.x - aa.x);
  distance += std::abs(bb.y - aa.y);
  return distance;
}

bool wars::Game::areAllies(int playerNumber1, int playerNumber2) const
{
  if(playerNumber1 == 0)
  {
    return playerNumber2 == 0;
  }
  else if(playerNumber2 == 0)
  {
    return false;
  }
  else
  {
    Player const& player1 = players.at(playerNumber1);
    Player const& player2 = players.at(playerNumber2);
    return player1.teamNumber == player2.teamNumber;
  }
}

wars::Game::Path wars::Game::findShortestPath(const wars::Game::Coordinates& a, const wars::Game::Coordinates& b) const
{
  std::map<Coordinates, Tile const*> grid;
  for(auto const& item : tiles)
  {
    Coordinates pos = {item.second.x, item.second.y};
    grid[pos] = &item.second;
  }

  if(grid.find(a) == grid.end() || grid.find(b) == grid.end())
  {
    return {};
  }
  typedef std::tuple<int, Coordinates, Coordinates, int> Node; // distance, tile, from, cost
  std::vector<Node> nodes = {std::make_tuple(calculateDistance(a, b), a, a, 0)};

  std::map<Coordinates, Node> visited;
  Path path;

  bool newNodes = false;
  while(!nodes.empty())
  {
    // Get node
    Node node = nodes.back();
    nodes.pop_back();

    // Add node to visited
    Coordinates pos = {std::get<1>(node).x, std::get<1>(node).y};
    visited[pos] = node;

    // Check end condition
    if(pos == b)
    {
      Node const* n = &node;
      while(n != nullptr)
      {
        path.push_back(std::get<1>(*n));
        n = std::get<1>(*n) == std::get<2>(*n) ? nullptr : &visited.at(std::get<2>(*n));
      }
      std::reverse(path.begin(), path.end());
      break;
    }
    // Process neighbors
    for(Coordinates neighborPos : neighborCoordinates(pos))
    {
      auto iter = grid.find(neighborPos);

      // Reject if does not exist
      if(iter == grid.end())
        continue;

      // Reject if visited
      if(visited.find(neighborPos) != visited.end())
        continue;

      // Check if already in queue
      auto existingIter = std::find_if(nodes.begin(), nodes.end(), [&neighborPos](Node const& n) {
        return std::get<1>(n) == neighborPos;
      });

      // Determine cost
      int cost = std::get<3>(node) + 1;

      if(existingIter == nodes.end())
      {
        // Add node to queue if new position
        nodes.push_back(std::make_tuple(calculateDistance(neighborPos, b), neighborPos, pos, cost));
        newNodes = true;
      }
      else if(cost < std::get<3>(*existingIter))
      {
        // Update existing if shorter route
        *existingIter = std::make_tuple(calculateDistance(neighborPos, b), neighborPos, pos, cost);
        newNodes = true;
      }
    }

    if(newNodes)
    {
      std::stable_sort(nodes.begin(), nodes.end());
    }
  }

  return path;
}

wars::Game::Path wars::Game::findUnitPath(const std::string& unitId, const wars::Game::Coordinates& destination) const
{
  Unit const& unit = getUnit(unitId);
  Tile const& startTile = getTile(unit.tileId);
  Coordinates const start = {startTile.x, startTile.y};
  UnitType const& unitType = rules.unitTypes.at(unit.type);
  MovementType const& movementType = rules.movementTypes.at(unitType.movementType);

  std::map<Coordinates, Tile const*> grid;
  for(auto const& item : tiles)
  {
    Coordinates pos = {item.second.x, item.second.y};
    grid[pos] = &item.second;
  }

  if(grid.find(destination) == grid.end())
  {
    return {};
  }
  typedef std::tuple<int, Coordinates, Coordinates, int> Node; // distance, tile, from, cost
  std::vector<Node> nodes = {std::make_tuple(calculateDistance(start, destination), start, start, 0)};

  std::map<Coordinates, Node> visited;
  Path path;

  bool newNodes = false;
  while(!nodes.empty())
  {
    // Get node
    Node node = nodes.back();
    nodes.pop_back();

    // Add node to visited
    Coordinates pos = {std::get<1>(node).x, std::get<1>(node).y};
    visited[pos] = node;

    // Check end condition
    if(pos == destination)
    {
      Node const* n = &node;
      while(n != nullptr)
      {
        path.push_back(std::get<1>(*n));
        n = std::get<1>(*n) == std::get<2>(*n) ? nullptr : &visited.at(std::get<2>(*n));
      }
      std::reverse(path.begin(), path.end());
      break;
    }
    // Process neighbors
    for(Coordinates neighborPos : neighborCoordinates(pos))
    {
      auto iter = grid.find(neighborPos);

      // Reject if does not exist
      if(iter == grid.end())
        continue;

      // Reject if visited
      if(visited.find(neighborPos) != visited.end())
        continue;

      // Determine cost
      Tile const* tile = iter->second;
      int tileCost = 1;
      auto effectIter = movementType.effectMap.find(tile->type);
      if(effectIter != movementType.effectMap.end())
      {
        tileCost = effectIter->second;
      }

      // Reject if cannot traverse
      if(tileCost < 0)
        continue;

      int cost = std::get<3>(node) + tileCost;

      // Reject if not enough movement points
      if(cost > unitType.movement)
        continue;

      // Reject if contains enemy unit
      if(!tile->unitId.empty() && !areAllies(unit.owner, getUnit(tile->unitId).owner))
        continue;

      // Check if already in queue
      auto existingIter = std::find_if(nodes.begin(), nodes.end(), [&neighborPos](Node const& n) {
        return std::get<1>(n) == neighborPos;
      });

      if(existingIter == nodes.end())
      {
        // Add node to queue if new position
        nodes.push_back(std::make_tuple(calculateDistance(neighborPos, destination), neighborPos, pos, cost));
        newNodes = true;
      }
      else if(cost < std::get<3>(*existingIter))
      {
        // Update existing if shorter route
        *existingIter = std::make_tuple(calculateDistance(neighborPos, destination), neighborPos, pos, cost);
        newNodes = true;
      }
    }

    if(newNodes)
    {
      std::stable_sort(nodes.begin(), nodes.end());
    }
  }

  return path;
}

std::vector<wars::Game::Coordinates> wars::Game::neighborCoordinates(const wars::Game::Coordinates& pos) const
{
  return {
    {pos.x + 1, pos.y},
    {pos.x - 1, pos.y},
    {pos.x, pos.y + 1},
    {pos.x, pos.y - 1},
    {pos.x + 1, pos.y - 1},
    {pos.x - 1, pos.y + 1},
  };
}

std::vector<wars::Game::Coordinates> wars::Game::findMovementOptions(const std::string& unitId) const
{
  Unit const& unit = getUnit(unitId);
  Tile const& startTile = getTile(unit.tileId);
  Coordinates const start = {startTile.x, startTile.y};
  UnitType const& unitType = rules.unitTypes.at(unit.type);
  MovementType const& movementType = rules.movementTypes.at(unitType.movementType);

  std::map<Coordinates, Tile const*> grid;
  for(auto const& item : tiles)
  {
    Coordinates pos = {item.second.x, item.second.y};
    grid[pos] = &item.second;
  }

  typedef std::tuple<int, Coordinates, Coordinates> Node; // cost, tile, from
  std::vector<Node> nodes = {std::make_tuple(0, start, start)};

  std::map<Coordinates, Node> visited;

  while(!nodes.empty())
  {
    // Get node
    Node node = nodes.back();
    nodes.pop_back();

    // Add node to visited
    Coordinates pos = {std::get<1>(node).x, std::get<1>(node).y};
    visited[pos] = node;

    // Process neighbors
    for(Coordinates neighborPos : neighborCoordinates(pos))
    {
      auto iter = grid.find(neighborPos);

      // Reject if does not exist
      if(iter == grid.end())
        continue;

      // Determine cost
      Tile const* tile = iter->second;
      int tileCost = 1;
      auto effectIter = movementType.effectMap.find(tile->type);
      if(effectIter != movementType.effectMap.end())
      {
        tileCost = effectIter->second;
      }

      // Reject if cannot traverse
      if(tileCost < 0)
        continue;

      int cost = std::get<0>(node) + tileCost;

      // Reject if not enough movement points
      if(cost > unitType.movement)
        continue;

      // Reject if contains enemy unit
      if(!tile->unitId.empty() && !areAllies(unit.owner, getUnit(tile->unitId).owner))
        continue;

      // Check if shorter route to already visited
      auto visitedIter = visited.find(neighborPos);
      if(visitedIter != visited.end() && std::get<0>(visitedIter->second) < cost)
        continue;

      // Check if already in queue
      auto existingIter = std::find_if(nodes.begin(), nodes.end(), [&neighborPos](Node const& n) {
        return std::get<1>(n) == neighborPos;
      });

      if(existingIter == nodes.end())
      {
        // Add node to queue if new position
        nodes.push_back(std::make_tuple(cost, neighborPos, pos));
      }
      else if(cost < std::get<0>(*existingIter))
      {
        // Update existing if shorter route
        *existingIter = std::make_tuple(cost, neighborPos, pos);
      }
    }
  }

  std::vector<Coordinates> result;
  for(auto const& item : visited)
  {
    Coordinates const& pos = item.first;

    // Skip if tile has a unit that cannot carry this one and isn't self
    Tile const* tile = grid.at(pos);
    if(!tile->unitId.empty() && tile->unitId != unitId)
    {
      Unit const& tileUnit = getUnit(tile->unitId);
      UnitType const& tileUnitType = rules.unitTypes.at(tileUnit.type);
      if(tileUnit.owner != unit.owner
         || tileUnit.carriedUnits.size() >= tileUnitType.carryNum
         || tileUnitType.carryClasses.find(unitType.unitClass) == tileUnitType.carryClasses.end())
      {
        continue;
      }
    }

    result.push_back(pos);
  }

  return result;
}

int wars::Game::calculateWeaponPower(Weapon const& weapon, int armorId, int distance) const
{
  auto efficiencyIter = weapon.rangeMap.find(distance);
  if(efficiencyIter == weapon.rangeMap.end())
    return -1;

  auto powerIter = weapon.powerMap.find(armorId);
  if(powerIter == weapon.powerMap.end())
    return -1;

  return powerIter->second * efficiencyIter->second / 100;
}

int wars::Game::calculateAttackDamage(UnitType const& attackerType, int attackerHealth, bool attackerDeployed,
                                      UnitType const& targetType, int targetHealth, int distance, int targetTerrainId) const
{

  // Calculate best attack power
  int power = -1;
  int weaponIds[] = {attackerType.primaryWeapon, attackerType.secondaryWeapon};
  for(int weaponId : weaponIds)
  {
    if(weaponId < 0)
      continue;

    Weapon const& weapon = rules.weapons.at(weaponId);

    if(weapon.requireDeployed && !attackerDeployed)
      continue;

    int weaponPower = calculateWeaponPower(weapon, targetType.armor, distance);
    power = std::max(power, weaponPower);
  }

  // Reject if cannot attack
  if(power < 0)
    return -1;

  // Determine enemy defense
  TerrainType const& targetTerrain = rules.terrainTypes.at(targetTerrainId);
  auto defenseIter = targetType.defenseMap.find(targetTerrainId);
  int defense = defenseIter != targetType.defenseMap.end() ? defenseIter->second : targetTerrain.defense;

  // Calculate damage
  int damage = attackerHealth * power * (100 - (defense * targetHealth / 100)) / 100 / 100;

  // Minimum damage is 1
  return std::max(damage, 1);
}

std::unordered_map<std::string, int> wars::Game::findAttackOptions(const std::string& unitId, const wars::Game::Coordinates& position) const
{
  int minRange = -1;
  int maxRange = -1;

  Unit const& unit = getUnit(unitId);

  UnitType const& unitType = rules.unitTypes.at(unit.type);
  int weaponIds[] = {unitType.primaryWeapon, unitType.secondaryWeapon};

  // Determine range limits for usable weapons
  for(int weaponId : weaponIds)
  {
    if(weaponId < 0)
      continue;

    Weapon const* weapon = &rules.weapons.at(weaponId);

    if(weapon->requireDeployed && !unit.deployed)
      continue;

    for(auto const& item : weapon->rangeMap)
    {
      minRange = minRange >= 0 ? std::min(minRange, item.first) : item.first;
      maxRange = maxRange >= 0 ? std::max(maxRange, item.first) : item.first;
    }
  }

  // Return empty set if no usable weapons
  if(minRange < 0 || maxRange < 0)
    return {};

  // Find attackable units and damages
  std::unordered_map<std::string, int> result;
  for(auto const& item : tiles)
  {
    // Reject if no unit
    Tile const& enemyTile = item.second;
    if(enemyTile.unitId.empty())
      continue;

    // Reject if out of range
    int distance = calculateDistance(position, {enemyTile.x, enemyTile.y});
    if(distance < minRange && distance > maxRange)
      continue;

    // Reject if unit is ally
    Unit const& enemy = getUnit(enemyTile.unitId);
    if(areAllies(unit.owner, enemy.owner))
      continue;

    UnitType enemyType = rules.unitTypes.at(enemy.type);

    // Calculate damage
    int damage = calculateAttackDamage(unitType, unit.health, unit.deployed, enemyType, enemy.health, distance, enemyTile.type);

    // Add result if attack is possible
    if(damage >= 0)
      result[enemy.id] = damage;
  }

  return result;
}

bool wars::Game::unitCanLoadInto(const std::string& unitId, const std::string& carrierId) const
{
  if(unitId.empty() || carrierId.empty() || unitId == carrierId)
    return false;

  Unit const& unit = getUnit(unitId);
  Unit const& carrier = getUnit(carrierId);
  UnitType const& unitType = rules.unitTypes.at(unit.type);
  UnitType const& carrierType = rules.unitTypes.at(carrier.type);

  return carrier.owner == unit.owner
      && carrier.carriedUnits.size() < carrierType.carryNum
      && carrierType.carryClasses.find(unitType.unitClass) != carrierType.carryClasses.end();
}

bool wars::Game::unitCanAttackFromTile(const std::string& unitId, const std::string& tileId) const
{
  Tile const& tile = getTile(tileId);

  if(!tile.unitId.empty() && tile.unitId != unitId)
    return false;

  return !findAttackOptions(unitId, {tile.x, tile.y}).empty();
}

bool wars::Game::unitCanCaptureTile(const std::string& unitId, const std::string& tileId) const
{
  Tile const& tile = getTile(tileId);

  if(!tile.unitId.empty() && tile.unitId != unitId)
    return false;

  Unit const& unit = getUnit(unitId);

  if(areAllies(unit.owner, tile.owner))
    return false;

  UnitType const& unitType = rules.unitTypes.at(unit.type);

  bool canCapture = false;
  for(int unitFlagId : unitType.flags)
  {
    UnitFlag const& flag = rules.unitFlags.at(unitFlagId);
    if(flag.name == "Capture")
    {
      canCapture = true;
      break;
    }
  }

  if(!canCapture)
    return false;

  TerrainType const& tileType = rules.terrainTypes.at(tile.type);

  bool capturable = false;
  for(int terrainFlagId : tileType.flags)
  {
    TerrainFlag const& flag = rules.terrainFlags.at(terrainFlagId);
    if(flag.name == "Capturable")
    {
      capturable = true;
      break;
    }
  }

  if(!capturable)
    return false;

  return true;
}

bool wars::Game::unitCanDeployAtTile(const std::string& unitId, const std::string& tileId) const
{
  Tile const& tile = getTile(tileId);

  if(!tile.unitId.empty())
    return false;

  Unit const& unit = getUnit(unitId);

  if(unit.deployed)
    return false;

  UnitType const& unitType = rules.unitTypes.at(unit.type);

  if((unitType.primaryWeapon < 0  || !rules.weapons.at(unitType.primaryWeapon).requireDeployed) &&
     (unitType.secondaryWeapon < 0  || !rules.weapons.at(unitType.secondaryWeapon).requireDeployed))
    return false;

  return true;
}

bool wars::Game::unitCanUndeploy(const std::string& unitId, const std::string& tileId) const
{
  Unit const& unit = getUnit(unitId);
  return unit.deployed;
}

bool wars::Game::unitCanUnloadAtTile(const std::string& unitId, const std::string& tileId) const
{
  Unit const& unit = getUnit(unitId);

  if(unit.carriedUnits.empty())
    return false;

  Tile const& tile = getTile(tileId);
  std::vector<Coordinates> unloadCoordinates = neighborCoordinates({tile.x, tile.y});
  std::vector<Tile const*> unloadTiles;

  for(Coordinates const& c : unloadCoordinates)
  {
    Tile const* t = getTileAt(c.x, c.y);
    if(t != nullptr)
    {
      unloadTiles.push_back(t);
    }
  }

  for(std::string carriedId : unit.carriedUnits)
  {
    Unit const& carried = getUnit(carriedId);
    UnitType const& carriedType = rules.unitTypes.at(carried.type);
    MovementType const& carriedMovementType = rules.movementTypes.at(carriedType.movementType);

    for(Tile const* t : unloadTiles)
    {
      auto effectIter = carriedMovementType.effectMap.find(t->type);
      if(effectIter == carriedMovementType.effectMap.end() || effectIter->second >= 0)
      {
        return true;
      }
    }
  }

  return false;
}

bool wars::Game::unitCanUnloadUnitFromTileToCoordinates(const std::string& unitId, const std::string& carriedId, const std::string& tileId, const wars::Game::Coordinates& destination) const
{
  Unit const& unit = getUnit(unitId);

  // Reject if carried is not being carried by unit
  if(std::find(unit.carriedUnits.begin(), unit.carriedUnits.end(), carriedId) == unit.carriedUnits.end())
    return false;

  Tile const& unloadTile = getTile(tileId);

  // Reject if tiles are not adjacent
  if(calculateDistance({unloadTile.x, unloadTile.y}, destination) != 1)
    return false;

  Unit const& carried = getUnit(carriedId);
  UnitType const& carriedType = rules.unitTypes.at(carried.type);
  MovementType const& carriedMovementType = rules.movementTypes.at(carriedType.movementType);

  // Reject if carried can't move on unload terrain
  auto unloadTileEffectIter = carriedMovementType.effectMap.find(unloadTile.type);
  if(unloadTileEffectIter != carriedMovementType.effectMap.end() && unloadTileEffectIter->second < 0)
    return false;

  // Reject if carried can't move on destination terrain
  Tile const& destinationTile = *getTileAt(destination.x, destination.y);
  auto destinationTileEffectIter = carriedMovementType.effectMap.find(destinationTile.type);
  if(destinationTileEffectIter != carriedMovementType.effectMap.end() && destinationTileEffectIter->second < 0)
    return false;

  return true;
}

std::vector<wars::Game::Coordinates> wars::Game::unitUnloadUnitFromTileOptions(std::string const& unitId, std::string const& carriedId, std::string const& tileId) const
{
  Unit const& unit = getUnit(unitId);

  // Reject if carried is not being carried by unit
  if(std::find(unit.carriedUnits.begin(), unit.carriedUnits.end(), carriedId) == unit.carriedUnits.end())
    return {};

  Tile const& tile = getTile(tileId);
  std::vector<Coordinates> unloadCoordinates = neighborCoordinates({tile.x, tile.y});
  std::vector<Tile const*> unloadTiles;

  // Determine adjacent tiles
  for(Coordinates const& c : unloadCoordinates)
  {
    Tile const* t = getTileAt(c.x, c.y);
    if(t != nullptr)
    {
      unloadTiles.push_back(t);
    }
  }


  Unit const& carried = getUnit(carriedId);
  UnitType const& carriedType = rules.unitTypes.at(carried.type);
  MovementType const& carriedMovementType = rules.movementTypes.at(carriedType.movementType);

  // Find tiles carried can be unloaded to
  std::vector<Coordinates> result;
  for(Tile const* t : unloadTiles)
  {
    auto effectIter = carriedMovementType.effectMap.find(t->type);
    if(effectIter == carriedMovementType.effectMap.end() || effectIter->second >= 0)
    {
      result.push_back({t->x, t->y});
    }
  }

  return result;
}

std::string wars::Game::updateTileFromJSON(const json::Value& value)
{
  Tile tile;
  tile.id = value.get("tileId").stringValue();
  tile.x = value.get("x").longValue();
  tile.y = value.get("y").longValue();
  tile.type = value.get("type").longValue();
  tile.subtype = value.get("subtype").longValue();
  tile.owner = value.get("owner").longValue();
  tile.capturePoints = value.get("capturePoints").longValue();
  tile.beingCaptured = value.get("beingCaptured").booleanValue();
  tile.unitId = parseStringOrNull(value.get("unitId"), "");

  if(!tile.unitId.empty())
  {
    updateUnitFromJSON(value.get("unit"));
  }

  tiles[tile.id] = tile;
  return tile.id;
}

std::string wars::Game::updateUnitFromJSON(const json::Value& value)
{
  std::string unitId = value.get("unitId").stringValue();

  auto iter = units.find(unitId);
  if(iter == units.end())
  {
    Unit u;
    u.id = unitId;
    units[unitId] = u;
    u.health = 100;
    u.deployed = false;
    u.capturing = false;
  }
  Unit& unit =  units[unitId];

  if(value.has("owner"))
    unit.owner = value.get("owner").longValue();
  if(value.has("type"))
    unit.type = value.get("type").longValue();
  if(value.has("tileId"))
    unit.tileId = parseStringOrNull(value.get("tileId"), "");
  if(value.has("carriedBy"))
    unit.carriedBy = parseStringOrNull(value.get("carriedBy"), "");
  if(value.has("health"))
    unit.health = value.get("health").longValue();
  if(value.has("deployed"))
    unit.deployed = value.get("deployed").booleanValue();
  if(value.has("moved"))
    unit.moved = value.get("moved").booleanValue();
  if(value.has("capturing"))
    unit.capturing = value.get("capturing").booleanValue();

  if(value.has("carriedUnits"))
  {
    json::Value carriedUnits = value.get("carriedUnits");
    unsigned int numCarriedUnits = carriedUnits.size();
    for(unsigned int i = 0; i < numCarriedUnits; ++i)
    {
      json::Value carriedUnit = carriedUnits.at(i);
      std::string carriedUnitId = updateUnitFromJSON(carriedUnit);
      unit.carriedUnits.push_back(carriedUnitId);
    }
  }
  return unit.id;
}

int wars::Game::updatePlayerFromJSON(const json::Value& value)
{
  int playerNumber = value.get("playerNumber").longValue();

  auto iter = players.find(playerNumber);
  if(iter == players.end())
  {
    Player p;
    p.playerNumber = playerNumber;
    players[playerNumber] = p;
  }
  Player& player =  players[playerNumber];

  if(value.has("_id"))
    player.id = value.get("_id").stringValue();
  if(value.has("userId"))
    player.userId = parseStringOrNull(value.get("userId"), "");
  if(value.has("playerName"))
    player.playerName = parseStringOrNull(value.get("playerName"), "");
  if(value.has("teamNumber"))
    player.teamNumber = value.get("teamNumber").longValue();
  if(value.get("funds").type() == json::Value::Type::NUMBER)
    player.funds = value.get("funds").longValue();
  if(value.has("score"))
    player.score = value.get("score").longValue();
  if(value.has("isMe"))
    player.isMe = value.get("isMe").booleanValue();
  if(value.get("settings").type() == json::Value::Type::OBJECT)
  {
    json::Value settings = value.get("settings");
    if(settings.has("emailNotifications"))
      player.emailNotifications = settings.get("emailNotifications").booleanValue();
    if(settings.has("hidden"))
      player.hidden = settings.get("hidden").booleanValue();
  }

  return playerNumber;
}

namespace
{
  template<typename T>
  std::unordered_map<int, T> parseAll(json::Value const& v)
  {
    std::unordered_map<int, T> result;
    std::vector<std::string> ids = v.properties();
    for(std::string id : ids)
    {
      json::Value json = v.get(id.data());
      T t = parse<T>(json);
      result[t.id] = t;
    }
    return result;
  }

  template<>
  wars::Weapon parse(json::Value const& v)
  {
    wars::Weapon weapon;
    weapon.id = v.get("id").longValue();
    weapon.name = v.get("name").stringValue();
    weapon.requireDeployed = v.get("requireDeployed").booleanValue();

    weapon.powerMap = parseIntIntMap(v.get("powerMap"));
    weapon.rangeMap = parseIntIntMap(v.get("rangeMap"));

    return weapon;
  }

  std::unordered_map<int, int> parseIntIntMap(json::Value const& v)
  {
    std::vector<std::string> props = v.properties();
    std::unordered_map<int, int> result;
    for(std::string const& s : props)
    {
      int i = 0;
      std::istringstream(s) >> i;
      result[i] = v.get(s).longValue();
    }

    return result;
  }

  std::unordered_map<int, int> parseIntIntMapWithNulls(json::Value const& v, int nullValue)
  {
    std::vector<std::string> props = v.properties();
    std::unordered_map<int, int> result;
    for(std::string const& s : props)
    {
      int i = 0;
      std::istringstream(s) >> i;
      json::Value prop = v.get(s);
      if(prop.type() == json::Value::Type::NULL_JSON)
      {
        result[i] = nullValue;
      }
      else
      {
        result[i] = prop.longValue();
      }
    }

    return result;
  }

  template<>
  wars::Armor parse(json::Value const& v)
  {
    wars::Armor armor;
    armor.id = v.get("id").longValue();
    armor.name = v.get("name").stringValue();
    return armor;
  }
  template<>
  wars::UnitClass parse(json::Value const& v)
  {
    wars::UnitClass value;
    value.id = v.get("id").longValue();
    value.name = v.get("name").stringValue();
    return value;
  }

  template<>
  wars::TerrainFlag parse(json::Value const& v)
  {
    wars::TerrainFlag value;
    value.id = v.get("id").longValue();
    value.name = v.get("name").stringValue();
    return value;
  }

  template<>
  wars::TerrainType parse(json::Value const& v)
  {
    wars::TerrainType value;
    value.id = v.get("id").longValue();
    value.name = v.get("name").stringValue();
    value.buildTypes = parseIntSet(v.get("buildTypes"));
    value.repairTypes = parseIntSet(v.get("repairTypes"));
    value.flags = parseIntSet(v.get("flags"));
    return value;
  }

  template<>
  wars::MovementType parse(json::Value const& v)
  {
    wars::MovementType value;
    value.id = v.get("id").longValue();
    value.name = v.get("name").stringValue();
    value.effectMap = parseIntIntMapWithNulls(v.get("effectMap"), -1);
    return value;
  }

  template<>
  wars::UnitFlag parse(json::Value const& v)
  {
    wars::UnitFlag value;
    value.id = v.get("id").longValue();
    value.name = v.get("name").stringValue();
    return value;
  }

  template<>
  wars::UnitType parse(json::Value const& v)
  {
    wars::UnitType value;
    value.id = v.get("id").longValue();
    value.name = v.get("name").stringValue();
    value.unitClass = v.get("unitClass").longValue();
    value.price = v.get("price").longValue();
    value.primaryWeapon = parseIntOrNull(v.get("primaryWeapon"), -1);
    value.secondaryWeapon = parseIntOrNull(v.get("secondaryWeapon"), -1);
    value.armor = v.get("armor").longValue();
    value.defenseMap = parseIntIntMap(v.get("defenseMap"));
    value.movementType = v.get("movementType").longValue();
    value.movement = v.get("movement").longValue();
    value.carryClasses = parseIntSet(v.get("carryClasses"));
    value.carryNum = v.get("carryNum").longValue();
    value.flags = parseIntSet(v.get("flags"));

    return value;
  }

  template<>
  wars::Rules parse(json::Value const& value)
  {
    wars::Rules rules;
    rules.weapons = parseAll<wars::Weapon>(value.get("weapons"));
    rules.armors = parseAll<wars::Armor>(value.get("armors"));
    rules.unitClasses = parseAll<wars::UnitClass>(value.get("unitClasses"));
    rules.terrainFlags = parseAll<wars::TerrainFlag>(value.get("terrainFlags"));
    rules.terrainTypes = parseAll<wars::TerrainType>(value.get("terrains"));
    rules.movementTypes = parseAll<wars::MovementType>(value.get("movementTypes"));
    rules.unitFlags = parseAll<wars::UnitFlag>(value.get("unitFlags"));
    rules.unitTypes = parseAll<wars::UnitType>(value.get("units"));
    return rules;
  }

  std::unordered_set<int> parseIntSet(json::Value const& v)
  {
    std::unordered_set<int> result;
    for(unsigned int i = 0; i < v.size(); ++i)
    {
      result.insert(v.at(i).longValue());
    }
    return result;
  }
  int parseIntOrNull(json::Value const& v, int nullValue)
  {
    if(v.type() == json::Value::Type::NULL_JSON)
    {
      return nullValue;
    }
    else
    {
      return v.longValue();
    }
  }
  std::string parseStringOrNull(json::Value const& v, std::string const& nullValue)
  {
    if(v.type() == json::Value::Type::NULL_JSON)
    {
      return nullValue;
    }
    else
    {
      return v.stringValue();
    }

  }
  wars::Game::Path parsePath(json::Value const& v)
  {
    wars::Game::Path path;
    unsigned int numCoordinates = v.size();
    for(int i = 0; i < numCoordinates; ++i)
    {
      json::Value value = v.at(i);
      int x = value.get("x").longValue();
      int y = value.get("y").longValue();
      path.push_back({x, y});
    }
    return path;
  }
}


bool wars::Game::Coordinates::operator<(const wars::Game::Coordinates& other) const
{
  return y != other.y ? y < other.y : x < other.x;
}

bool wars::Game::Coordinates::operator==(const wars::Game::Coordinates& other) const
{
  return x == other.x && y == other.y;
}
