#include "game.h"
#include <iostream>
#include <sstream>

#include "jsonpp.h"

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
}
wars::Game::Game(): gameId(), authorId(),  name(), mapId(),
  state(0), turnStart(0), turnNumber(0), roundNumber(0), inTurnNumber(0),
  publicGame(false), turnLength(0), bannedUnits(0)
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
}
