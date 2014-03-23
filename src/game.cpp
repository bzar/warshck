#include "game.h"
#include <iostream>
#include <sstream>

#include "jsonpp.h"

std::unordered_map<std::string, wars::Game::State> const wars::Game::STATE_NAMES = {
  {"pregame", State::PREGAME},
  {"inProgress", State::IN_PROGRESS},
  {"finished", State::FINISHED}
};

namespace
{
  template<typename T>
  T parse(JSONValue const& v);

  template<typename T>
  std::unordered_map<int, T> parseAll(JSONValue const& v);

  template<>
  wars::Weapon parse(JSONValue const& v);
  template<>
  wars::Armor parse(JSONValue const& v);
  template<>
  wars::UnitClass parse(JSONValue const& v);
  template<>
  wars::TerrainFlag parse(JSONValue const& v);
  template<>
  wars::TerrainType parse(JSONValue const& v);
  template<>
  wars::MovementType parse(JSONValue const& v);
  template<>
  wars::UnitFlag parse(JSONValue const& v);
  template<>
  wars::UnitType parse(JSONValue const& v);
  template<>
  wars::Rules parse(JSONValue const& value);

  std::unordered_map<int, int> parseIntIntMap(JSONValue const& v);
  std::unordered_map<int, int> parseIntIntMapWithNulls(JSONValue const& v, int nullValue);
  std::unordered_set<int> parseIntSet(JSONValue const& v);
  int parseIntOrNull(JSONValue const& v, int nullValue);
  std::string parseStringOrNull(JSONValue const& v, std::string const& nullValue);
  wars::Game::Path parsePath(JSONValue const& v);
}
wars::Game::Game(): gameId(), authorId(),  name(), mapId(),
  state(State::PREGAME), turnStart(0), turnNumber(0), roundNumber(0), inTurnNumber(0),
  publicGame(false), turnLength(0), bannedUnits(0),
  rules(), tiles(), units(),  players()
{

}

wars::Game::~Game()
{

}

void wars::Game::setRulesFromJSON(const JSONValue& value)
{
  rules = parse<Rules>(value);
}

void wars::Game::setGameDataFromJSON(const JSONValue& value)
{
  JSONValue game = value.get("game");
  gameId = game.get("gameId").stringValue();
  authorId = game.get("authorId").stringValue();
  name = game.get("name").stringValue();
  mapId = game.get("mapId").stringValue();
  state = STATE_NAMES.at(game.get("state").stringValue());
  turnStart = game.get("turnStart").numberValue();
  turnNumber = game.get("turnNumber").numberValue();
  roundNumber = game.get("roundNumber").numberValue();
  inTurnNumber = game.get("inTurnNumber").numberValue();

  JSONValue settings = game.get("settings");
  publicGame = settings.get("public").booleanValue();
  turnLength = parseIntOrNull(settings.get("turnLength"), -1);
  bannedUnits = parseIntSet(settings.get("bannedUnits"));

  JSONValue tileArray = game.get("tiles");
  unsigned int numTiles = tileArray.size();
  for(unsigned int i = 0; i < numTiles; ++i)
  {
    JSONValue tile = tileArray.at(i);
    updateTileFromJSON(tile);
  }

}

void wars::Game::processEventFromJSON(const JSONValue& value)
{
  JSONValue content = value.get("content");
  std::string action = content.get("action").stringValue();
  /*when "move" then map.moveUnit e.unit.unitId, e.tile.tileId, e.path, nextEvent
    when "wait" then map.waitUnit e.unit.unitId, nextEvent
    when "attack" then map.attackUnit e.attacker.unitId, e.target.unitId, e.damage, nextEvent
    when "counterattack" then map.counterattackUnit e.attacker.unitId, e.target.unitId, e.damage, nextEvent
    when "capture" then map.captureTile e.unit.unitId, e.tile.tileId, e.left, nextEvent
    when "captured" then map.capturedTile e.unit.unitId, e.tile.tileId, nextEvent
    when "deploy" then map.deployUnit e.unit.unitId, nextEvent
    when "undeploy" then map.undeployUnit e.unit.unitId, nextEvent
    when "load" then map.loadUnit e.unit.unitId, e.carrier.unitId, nextEvent
    when "unload" then map.unloadUnit e.unit.unitId, e.carrier.unitId, e.tile.tileId, nextEvent
    when "destroyed" then map.destroyUnit e.unit.unitId, nextEvent
    when "repair" then map.repairUnit e.unit.unitId, e.newHealth, nextEvent
    when "build" then map.buildUnit e.tile.tileId, e.unit, nextEvent
    when "regenerateCapturePoints" then map.regenerateCapturePointsTile e.tile.tileId, e.newCapturePoints, nextEvent
    when "produceFunds" then map.produceFundsTile e.tile.tileId, nextEvent
    when "beginTurn" then map.beginTurn e.player, nextEvent
    when "endTurn" then map.endTurn e.player, nextEvent
    when "turnTimeout" then map.turnTimeout e.player, nextEvent
    when "finished" then map.finished e.winner, nextEvent
    when "surrender" then map.surrender e.player, nextEvent*/

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
    int damage = content.get("damage").numberValue();
    attackUnit(attackerId, targetId, damage);
  }
  else if(action == "counterattack")
  {
    std::string attackerId = content.get("attacker").get("unitId").stringValue();
    std::string targetId = content.get("target").get("unitId").stringValue();
    int damage = content.get("damage").numberValue();
    counterattackUnit(attackerId, targetId, damage);
  }
  else if(action == "capture")
  {
    std::string unitId = content.get("unit").get("unitId").stringValue();
    std::string tileId = content.get("tile").get("tileId").stringValue();
    int left = content.get("left").numberValue();
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
    int newHealth = content.get("newHealth").numberValue();
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
    int newCapturePoints = content.get("newCapturePoints").numberValue();
    regenerateCapturePointsTile(tileId, newCapturePoints);
  }
  else if(action == "produceFunds")
  {
    std::string tileId = content.get("tile").get("tileId").stringValue();
    produceFundsTile(tileId);
  }
  else if(action == "beginTurn")
  {
    int playerNumber = content.get("player").numberValue();
    beginTurn(playerNumber);
  }
  else if(action == "endTurn")
  {
    int playerNumber = content.get("player").numberValue();
    endTurn(playerNumber);
  }
  else if(action == "turnTimeout")
  {
    int playerNumber = content.get("player").numberValue();
    turnTimeout(playerNumber);
  }
  else if(action == "finished")
  {
    int winnerPlayerNumber = content.get("winner").numberValue();
    finished(winnerPlayerNumber);
  }
  else if(action == "surrender")
  {
    int playerNumber = content.get("player").numberValue();
    surrender(playerNumber);
  }
  else
  {
    std::cerr << "Unknown event action: " << action << std::endl;
  }
}

void wars::Game::processEventsFromJSON(const JSONValue& value)
{
  unsigned int numEvents = value.size();
  for(int i = 0; i < numEvents; ++i)
  {
    JSONValue event = value.at(i);
    processEventFromJSON(event);
  }
}

void wars::Game::moveUnit(std::string const& unitId, std::string const& tileId, Path const& path)
{
  std::cout << "void wars::Game::moveUnit(std::string const& unitId, std::string const& tileId, Path const& path)" << std::endl;
}

void wars::Game::waitUnit(std::string const& unitId)
{
  std::cout << "void wars::Game::waitUnit(std::string const& unitId)" << std::endl;

}

void wars::Game::attackUnit(std::string const& attackerId, std::string const& targetId, int damage)
{
  std::cout << "void wars::Game::attackUnit(std::string const& attackerId, std::string const& targetId, int damage)" << std::endl;

}

void wars::Game::counterattackUnit(std::string const& attackerId, std::string const& targetId, int damage)
{
  std::cout << "void wars::Game::counterattackUnit(std::string const& attackerId, std::string const& targetId, int damage)" << std::endl;

}

void wars::Game::captureTile(std::string const& unitId, std::string const& tileId, int left)
{
  std::cout << "void wars::Game::captureTile(std::string const& unitId, std::string const& tileId, int left)" << std::endl;

}

void wars::Game::capturedTile(std::string const& unitId, std::string const& tileId)
{
  std::cout << "void wars::Game::capturedTile(std::string const& unitId, std::string const& tileId)" << std::endl;

}

void wars::Game::deployUnit(std::string const& unitId)
{
  std::cout << "void wars::Game::deployUnit(std::string const& unitId)" << std::endl;

}

void wars::Game::undeployUnit(std::string const& unitId)
{
  std::cout << "void wars::Game::undeployUnit(std::string const& unitId)" << std::endl;

}

void wars::Game::loadUnit(std::string const& unitId, std::string const& carrierId)
{
  std::cout << "void wars::Game::loadUnit(std::string const& unitId, std::string const& carrierId)" << std::endl;

}

void wars::Game::unloadUnit(std::string const& unitId, std::string const& carrierId, std::string const& tileId)
{
  std::cout << "void wars::Game::unloadUnit(std::string const& unitId, std::string const& carrierId, std::string const& tileId)" << std::endl;

}

void wars::Game::destroyUnit(std::string const& unitId)
{
  std::cout << "void wars::Game::destroyUnit(std::string const& unitId)" << std::endl;

}

void wars::Game::repairUnit(std::string const& unitId, int newHealth)
{
  std::cout << "void wars::Game::repairUnit(std::string const& unitId, int newHealth)" << std::endl;

}

void wars::Game::buildUnit(std::string const& tileId, std::string const& unitId)
{
  std::cout << "void wars::Game::buildUnit(std::string const& tileId, std::string const& unitId)" << std::endl;

}

void wars::Game::regenerateCapturePointsTile(std::string const& tileId, int newCapturePoints)
{
  std::cout << "void wars::Game::regenerateCapturePointsTile(std::string const& tileId, int newCapturePoints)" << std::endl;

}

void wars::Game::produceFundsTile(std::string const& tileId)
{
  std::cout << "void wars::Game::produceFundsTile(d::string const& tileId)" << std::endl;

}

void wars::Game::beginTurn(int playerNumber)
{
  std::cout << "void wars::Game::beginTurn(int playerNumber)" << std::endl;

}

void wars::Game::endTurn(int playerNumber)
{
  std::cout << "void wars::Game::endTurn(int playerNumber)" << std::endl;

}

void wars::Game::turnTimeout(int playerNumber)
{
  std::cout << "void wars::Game::turnTimeout(int playerNumber)" << std::endl;

}

void wars::Game::finished(int winnerPlayerNumber)
{
  std::cout << "void wars::Game::finished(int winnerPlayerNumber)" << std::endl;

}

void wars::Game::surrender(int playerNumber)
{
  std::cout << "void wars::Game::surrender(int playerNumber)" << std::endl;

}

std::string wars::Game::updateTileFromJSON(const JSONValue& value)
{
  //  std::cout << value.toString() << std::endl;
  Tile tile;
  tile.id = value.get("tileId").stringValue();
  tile.x = value.get("x").numberValue();
  tile.y = value.get("y").numberValue();
  tile.type = value.get("type").numberValue();
  tile.subtype = value.get("subtype").numberValue();
  tile.owner = value.get("owner").numberValue();
  tile.capturePoints = value.get("capturePoints").numberValue();
  tile.beingCaptured = value.get("beingCaptured").booleanValue();
  tile.unitId = parseStringOrNull(value.get("unitId"), "");

  if(!tile.unitId.empty())
  {
    updateUnitFromJSON(value.get("unit"));
  }

  tiles[tile.id] = tile;
  return tile.id;
}

std::string wars::Game::updateUnitFromJSON(const JSONValue& value)
{
  std::string unitId = value.get("unitId").stringValue();

  auto iter = units.find(unitId);
  if(iter == units.end())
  {
    Unit u;
    u.id = unitId;
    units[unitId] = u;
  }
  Unit& unit =  units[unitId];

  if(value.get("owner").type() != JSONValue::Type::NONE)
    unit.owner = value.get("owner").numberValue();
  if(value.get("type").type() != JSONValue::Type::NONE)
    unit.type = value.get("type").numberValue();
  if(value.get("tileId").type() != JSONValue::Type::NONE)
    unit.tileId = parseStringOrNull(value.get("tileId"), "");
  if(value.get("carriedBy").type() != JSONValue::Type::NONE)
    unit.carriedBy = parseStringOrNull(value.get("carriedBy"), "");
  if(value.get("health").type() != JSONValue::Type::NONE)
    unit.health = value.get("health").numberValue();
  if(value.get("deployed").type() != JSONValue::Type::NONE)
    unit.deployed = value.get("deployed").booleanValue();
  if(value.get("moved").type() != JSONValue::Type::NONE)
    unit.moved = value.get("moved").booleanValue();
  if(value.get("capturing").type() != JSONValue::Type::NONE)
    unit.capturing = value.get("capturing").booleanValue();

  if(value.get("carriedUnits").type() != JSONValue::Type::NONE)
  {
    JSONValue carriedUnits = value.get("carriedUnits");
    unsigned int numCarriedUnits = carriedUnits.size();
    for(unsigned int i = 0; i < numCarriedUnits; ++i)
    {
      JSONValue carriedUnit = carriedUnits.at(i);
      std::string carriedUnitId = updateUnitFromJSON(carriedUnit);
      unit.carriedUnits.push_back(carriedUnitId);
    }
  }
  return unit.id;
}

namespace
{
  template<typename T>
  std::unordered_map<int, T> parseAll(JSONValue const& v)
  {
    std::unordered_map<int, T> result;
    std::vector<std::string> ids = v.properties();
    for(std::string id : ids)
    {
      JSONValue json = v.get(id.data());
      T t = parse<T>(json);
      result[t.id] = t;
    }
    return result;
  }

  template<>
  wars::Weapon parse(JSONValue const& v)
  {
    wars::Weapon weapon;
    weapon.id = v.get("id").numberValue();
    weapon.name = v.get("name").stringValue();
    weapon.requireDeployed = v.get("requireDeployed").booleanValue();

    weapon.powerMap = parseIntIntMap(v.get("powerMap"));
    weapon.rangeMap = parseIntIntMap(v.get("rangeMap"));

    return weapon;
  }

  std::unordered_map<int, int> parseIntIntMap(JSONValue const& v)
  {
    std::vector<std::string> props = v.properties();
    std::unordered_map<int, int> result;
    for(std::string const& s : props)
    {
      int i = 0;
      std::istringstream(s) >> i;
      result[i] = v.get(s).numberValue();
    }

    return result;
  }

  std::unordered_map<int, int> parseIntIntMapWithNulls(JSONValue const& v, int nullValue)
  {
    std::vector<std::string> props = v.properties();
    std::unordered_map<int, int> result;
    for(std::string const& s : props)
    {
      int i = 0;
      std::istringstream(s) >> i;
      JSONValue prop = v.get(s);
      if(prop.type() == JSONValue::Type::NULL_JSON)
      {
        result[i] = nullValue;
      }
      else
      {
        result[i] = prop.numberValue();
      }
    }

    return result;
  }

  template<>
  wars::Armor parse(JSONValue const& v)
  {
    wars::Armor armor;
    armor.id = v.get("id").numberValue();
    armor.name = v.get("name").stringValue();
    return armor;
  }
  template<>
  wars::UnitClass parse(JSONValue const& v)
  {
    wars::UnitClass value;
    value.id = v.get("id").numberValue();
    value.name = v.get("name").stringValue();
    return value;
  }

  template<>
  wars::TerrainFlag parse(JSONValue const& v)
  {
    wars::TerrainFlag value;
    value.id = v.get("id").numberValue();
    value.name = v.get("name").stringValue();
    return value;
  }

  template<>
  wars::TerrainType parse(JSONValue const& v)
  {
    wars::TerrainType value;
    value.id = v.get("id").numberValue();
    value.name = v.get("name").stringValue();
    value.buildTypes = parseIntSet(v.get("buildTypes"));
    value.repairTypes = parseIntSet(v.get("repairTypes"));
    value.flags = parseIntSet(v.get("flags"));
    return value;
  }

  template<>
  wars::MovementType parse(JSONValue const& v)
  {
    wars::MovementType value;
    value.id = v.get("id").numberValue();
    value.name = v.get("name").stringValue();
    value.effectMap = parseIntIntMapWithNulls(v.get("effectMap"), -1);
    return value;
  }

  template<>
  wars::UnitFlag parse(JSONValue const& v)
  {
    wars::UnitFlag value;
    value.id = v.get("id").numberValue();
    value.name = v.get("name").stringValue();
    return value;
  }

  template<>
  wars::UnitType parse(JSONValue const& v)
  {
    wars::UnitType value;
    value.id = v.get("id").numberValue();
    value.name = v.get("name").stringValue();
    value.unitClass = v.get("id").numberValue();
    value.price = v.get("price").numberValue();
    value.primaryWeapon = parseIntOrNull(v.get("primaryWeapon"), -1);
    value.secondaryWeapon = parseIntOrNull(v.get("secondaryWeapon"), -1);
    value.armor = v.get("armor").numberValue();
    value.defenseMap = parseIntIntMap(v.get("defenseMap"));
    value.movementType = v.get("movementType").numberValue();
    value.movement = v.get("movement").numberValue();
    value.carryClasses = parseIntSet(v.get("carryClasses"));
    value.carryNum = v.get("carryNum").numberValue();
    value.flags = parseIntSet(v.get("flags"));

    return value;
  }

  template<>
  wars::Rules parse(JSONValue const& value)
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

  std::unordered_set<int> parseIntSet(JSONValue const& v)
  {
    std::unordered_set<int> result;
    for(unsigned int i = 0; i < v.size(); ++i)
    {
      result.insert(v.at(i).numberValue());
    }
    return result;
  }
  int parseIntOrNull(JSONValue const& v, int nullValue)
  {
    if(v.type() == JSONValue::Type::NULL_JSON)
    {
      return nullValue;
    }
    else
    {
      return v.numberValue();
    }
  }
  std::string parseStringOrNull(JSONValue const& v, std::string const& nullValue)
  {
    if(v.type() == JSONValue::Type::NULL_JSON)
    {
      return nullValue;
    }
    else
    {
      return v.stringValue();
    }

  }
  wars::Game::Path parsePath(JSONValue const& v)
  {
    wars::Game::Path path;
    unsigned int numCoordinates = v.size();
    for(int i = 0; i < numCoordinates; ++i)
    {
      JSONValue value = v.at(i);
      int x = value.get("x").numberValue();
      int y = value.get("y").numberValue();
      path.push_back({x, y});
    }
    return path;
  }
}
