#ifndef WARS_GAME_H
#define WARS_GAME_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "rules.h"

class JSONValue;

namespace wars
{
  class Game
  {
  public:
    Game();
    ~Game();

    void setRulesFromJSON(JSONValue const& value);
    void setGameDataFromJSON(JSONValue const& value);

    struct Tile
    {
      std::string id;
      int x;
      int y;
      int type;
      int subtype;
      int owner;
      std::string unitId;
      int capturePoints;
      bool beingCaptured;
    };
    struct Unit
    {
      std::string id;
      std::string tileId;
      int type;
      int owner;
      std::string carriedBy;
      int health;
      bool deployed;
      bool moved;
      bool capturing;
      std::vector<std::string> carriedUnits;
    };

    struct Player
    {
      std::string id;
      std::string userId;
      int playerNumber;
      int teamNumber;
      int funds;
      int score;
      bool emailNotifications;
      bool hidden;
    };

  private:
    std::string gameId;
    std::string authorId;
    std::string name;
    std::string mapId;
    int state;
    long turnStart;
    int turnNumber;
    int roundNumber;
    int inTurnNumber;
    bool publicGame;
    long turnLength;
    std::unordered_set<int> bannedUnits;

    Rules rules;
  };
}
#endif // WARS_GAME_H
