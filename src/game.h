#ifndef WARS_GAME_H
#define WARS_GAME_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "rules.h"
#include "stream.h"

namespace json
{
   class Value;
}

namespace wars
{
  class Game
  {
  public:
    enum class State { PREGAME, IN_PROGRESS, FINISHED };
    struct Coordinates
    {
      bool operator<(Coordinates const& other) const;
      bool operator==(Coordinates const& other) const;

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
      std::string playerName;
      int playerNumber;
      int teamNumber;
      int funds;
      int score;
      bool emailNotifications;
      bool hidden;
      bool isMe;

      Player() : id(), userId(), playerName(), playerNumber(0), teamNumber(0), funds(0),
        score(0), emailNotifications(false), hidden(false), isMe(false)
      {}
    };

    Game();
    ~Game();

    Stream<Event> events();

    void setRulesFromJSON(json::Value const& value);
    void setGameDataFromJSON(json::Value const& value);
    void processEventFromJSON(json::Value const& value);
    void processEventsFromJSON(json::Value const& value);

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

    Player const& getInTurn();
    Tile const* getTileAt(int x, int y) const;

    std::string const& getGameId() const;

    int calculateDistance(Coordinates const& a, Coordinates const& b) const;
    bool areAllies(int playerNumber1, int playerNumber2) const;
    Path findShortestPath(Coordinates const& a, Coordinates const& b) const;
    Path findUnitPath(std::string const& unitId, Coordinates const& destination) const;
    std::vector<Coordinates> neighborCoordinates(Coordinates const& pos) const;
    std::vector<Coordinates> findMovementOptions(std::string const& unitId) const;
    int calculateWeaponPower(Weapon const& weapon, int armorId, int distance) const;
    int calculateAttackDamage(UnitType const& attackerType, int attackerHealth, bool attackerDeployed, UnitType const& targetType, int targetHealth, int distance, int targetTerrainId) const;
    std::unordered_map<std::string, int> findAttackOptions(std::string const& unitId, Coordinates const& position) const;
    bool unitCanLoadInto(std::string const& unitId, std::string const& carrierId) const;
    bool unitCanAttackFromTile(std::string const& unitId, std::string const& tileId);
    bool unitCanCaptureTile(std::string const& unitId, std::string const& tileId);
    bool unitCanDeployAtTile(std::string const& unitId, std::string const& tileId);
    bool unitCanUndeploy(std::string const& unitId, std::string const& tileId);
    bool unitCanUnloadAtTile(std::string const& unitId, std::string const& tileId);

  private:
    static std::unordered_map<std::string, State> const STATE_NAMES;

    std::string updateTileFromJSON(json::Value const& value);
    std::string updateUnitFromJSON(json::Value const& value);
    int updatePlayerFromJSON(json::Value const& value);

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
