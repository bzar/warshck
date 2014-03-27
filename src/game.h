#ifndef WARS_GAME_H
#define WARS_GAME_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "rules.h"
#include "stream.h"

class JSONValue;

namespace wars
{
  class Game
  {
  public:
    enum class State { PREGAME, IN_PROGRESS, FINISHED };
    struct Coordinates
    {
      int x;
      int y;
    };
    typedef std::vector<Coordinates> Path;
    static const int NEUTRAL_PLAYER_NUMBER = 0;

    enum class EventType {
      GAMEDATA, MOVE, WAIT, ATTACK, COUNTERATTACK, CAPTURE, CAPTURED,
      DEPLOY, UNDEPLOY, LOAD, UNLOAD, DESTROY, REPAIR, BUILD,
      REGENERATE_CAPTURE_POINTS, PRODUCE_FUNDS, BEGIN_TURN,
      END_TURN, TURN_TIMEOUT, FINISHED, SURRENDER
    };

    struct Event
    {
      EventType type;
      union
      {
        struct
        {
          std::string const* unitId;
          std::string const* tileId;
          Path const* path;
        } move;
        struct
        {
          std::string const* unitId;
        } wait;
        struct
        {
          std::string const* attackerId;
          std::string const* targetId;
          int damage;
        } attack;
        struct
        {
          std::string const* attackerId;
          std::string const* targetId;
          int damage;
        } counterattack;
        struct
        {
          std::string const* unitId;
          std::string const* tileId;
          int left;
        } capture;
        struct
        {
          std::string const* unitId;
          std::string const* tileId;
        } captured;
        struct
        {
          std::string const* unitId;
        } deploy;
        struct
        {
          std::string const* unitId;
        } undeploy;
        struct
        {
          std::string const* unitId;
          std::string const* carrierId;
        } load;
        struct
        {
          std::string const* unitId;
          std::string const* carrierId;
          std::string const* tileId;
        } unload;
        struct
        {
          std::string const* unitId;
        } destroy;
        struct
        {
          std::string const* unitId;
          int newHealth;
        } repair;
        struct
        {
          std::string const* tileId;
          std::string const* unitId;
        } build;
        struct
        {
          std::string const* tileId;
          int newCapturePoints;
        } regenerateCapturePoints;
        struct
        {
          std::string const* tileId;
        } produceFunds;
        struct
        {
          int playerNumber;
        } beginTurn;
        struct
        {
          int playerNumber;
        } endTurn;
        struct
        {
          int playerNumber;
        } turnTimeout;
        struct
        {
          int winnerPlayerNumber;
        } finished;
        struct
        {
          int playerNumber;
        } surrender;
      };
    };

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

      Tile() : id(), x(0), y(0), type(0), subtype(0), owner(0),
        unitId(), capturePoints(0), beingCaptured(false)
      {}
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

      Unit() : id(), tileId(), type(0), owner(0), carriedBy(), health(0),
        deployed(false), moved(false), capturing(false), carriedUnits()
      {}
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
      bool isMe;

      Player() : id(), userId(), playerNumber(0), teamNumber(0), funds(0),
        score(0), emailNotifications(false), hidden(false), isMe(false)
      {}
    };

    Game();
    ~Game();

    Stream<Event> events();

    void setRulesFromJSON(JSONValue const& value);
    void setGameDataFromJSON(JSONValue const& value);
    void processEventFromJSON(JSONValue const& value);
    void processEventsFromJSON(JSONValue const& value);

    // Game event handlers
    void moveUnit(std::string const& unitId, std::string const& tileId, Path const& path);
    void waitUnit(std::string const& unitId);
    void attackUnit(std::string const& attackerId, std::string const& targetId, int damage);
    void counterattackUnit(std::string const& attackerId, std::string const& targetId, int damage);
    void captureTile(std::string const& unitId, std::string const& tileId, int left);
    void capturedTile(std::string const& unitId, std::string const& tileId);
    void deployUnit(std::string const& unitId);
    void undeployUnit(std::string const& unitId);
    void loadUnit(std::string const& unitId, std::string const& carrierId);
    void unloadUnit(std::string const& unitId, std::string const& carrierId, std::string const& tileId);
    void destroyUnit(std::string const& unitId);
    void repairUnit(std::string const& unitId, int newHealth);
    void buildUnit(std::string const& tileId, std::string const& unitId);
    void regenerateCapturePointsTile(std::string const& tileId, int newCapturePoints);
    void produceFundsTile(std::string const& tileId);
    void beginTurn(int playerNumber);
    void endTurn(int playerNumber);
    void turnTimeout(int playerNumber);
    void finished(int winnerPlayerNumber);
    void surrender(int playerNumber);


    Tile const& getTile(std::string const& tileId) const;
    Unit const& getUnit(std::string const& unitId) const;
    Player const& getPlayer(int playerNumber) const;

    std::unordered_map<std::string, Tile> const& getTiles() const;
    std::unordered_map<std::string, Unit> const& getUnits() const;
    std::unordered_map<int, Player> const& getPlayers() const;
    Rules const& getRules() const;

  private:
    static std::unordered_map<std::string, State> const STATE_NAMES;

    std::string updateTileFromJSON(JSONValue const& value);
    std::string updateUnitFromJSON(JSONValue const& value);
    int updatePlayerFromJSON(JSONValue const& value);

    std::string gameId;
    std::string authorId;
    std::string name;
    std::string mapId;
    State state;
    double turnStart;
    int turnNumber;
    int roundNumber;
    int inTurnNumber;
    bool publicGame;
    double turnLength;
    std::unordered_set<int> bannedUnits;

    Rules rules;

    std::unordered_map<std::string, Tile> tiles;
    std::unordered_map<std::string, Unit> units;
    std::unordered_map<int, Player> players;

    Stream<Event> eventStream;
  };
}
#endif // WARS_GAME_H
