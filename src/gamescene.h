#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "glhck/glhck.h"
#include "theme.h"
#include "hexlabel.h"
#include "game.h"

#include <unordered_map>
#include <vector>
#include <string>

namespace wars
{
  class GameScene
  {
  public:
    GameScene(Game* game, Theme* theme);

    void render();

    kmVec3 hexToRect(kmVec3 const& v);
    kmVec3 rectToHex(kmVec3 const& v);

    void setHighlightedTiles(std::vector<Game::Coordinates> const& coords);
    void clearHighlightedTiles();
    void setAttackOptions(std::unordered_map<std::string, int> const& options);
    void clearAttackOptions(std::unordered_map<std::string, int> const& options);

  private:
    struct Unit
    {
      std::string id;
      glhckObject* obj;
      struct
      {
        bool highlight = false;
      } effects;
    };
    struct Tile
    {
      std::string id;
      glhckObject* hex;
      glhckObject* prop;
      bool labelUpdate;
      HexLabel label;

      struct
      {
        bool highlight = false;
      } effects;
    };

    void initializeFromGame();

    void updatePropTexture(glhckObject* o, int terrainId, int owner);
    void updateHexLabel(std::string const& id);

    glhckObject* createUnitObject(Game::Unit const& unit);
    glhckObject* createTileHex(Game::Tile const& tile);
    glhckObject* createTileProp(Game::Tile const& tile);

    Game* _game;
    Theme* _theme;
    glhckObject* _sky;

    std::unordered_map<std::string, Unit> _units;
    std::unordered_map<std::string, Tile> _tiles;

    Stream<wars::Game::Event>::Subscription _eventSub;
  };

}
#endif
