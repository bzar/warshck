#ifndef WARS_RULES_H
#define WARS_RULES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace wars
{
  struct Weapon
  {
    int id;
    std::string name;
    bool requireDeployed;
    std::unordered_map<int, int> rangeMap;
    std::unordered_map<int, int> powerMap;
  };

  struct Armor
  {
    int id;
    std::string name;
  };

  struct UnitClass
  {
    int id;
    std::string name;
  };

  struct TerrainFlag
  {
    int id;
    std::string name;
  };

  struct TerrainType
  {
    int id;
    std::string name;
    int defense;
    std::unordered_set<int> buildTypes;
    std::unordered_set<int> repairTypes;
    std::unordered_set<int> flags;
  };

  struct MovementType
  {
    int id;
    std::string name;
    std::unordered_map<int, int> effectMap;
  };

  struct UnitFlag
  {
    int id;
    std::string name;
  };

  struct UnitType
  {
    int id;
    std::string name;
    int unitClass;
    int price;
    int primaryWeapon;
    int secondaryWeapon;
    int armor;
    std::unordered_map<int, int> defenseMap;
    int movementType;
    int movement;
    std::unordered_set<int> carryClasses;
    int carryNum;
    std::unordered_set<int> flags;
  };

  struct Rules
  {
    std::unordered_map<int, Weapon> weapons;
    std::unordered_map<int, Armor> armors;
    std::unordered_map<int, UnitClass> unitClasses;
    std::unordered_map<int, TerrainFlag> terrainFlags;
    std::unordered_map<int, TerrainType> terrainTypes;
    std::unordered_map<int, MovementType> movementTypes;
    std::unordered_map<int, UnitFlag> unitFlags;
    std::unordered_map<int, UnitType> unitTypes;
  };
}
#endif // WARS_RULES_H
