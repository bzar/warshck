#ifndef WARS_GLHCKVIEW_H
#define WARS_GLHCKVIEW_H

#include "view.h"
#include "glhck/glhck.h"
#include "GLFW/glfw3.h"

#include <string>
#include <unordered_map>

namespace wars
{
  class GlhckView : public View
  {
  public:
    static void init(int argc, char** argv);
    static void term();

    GlhckView();
    ~GlhckView();

    void setGame(Game* game) override;
    bool handle() override;
    void quit();

  private:
    struct Unit
    {
      std::string id;
      glhckObject* obj;
    };
    struct Tile
    {
      std::string id;
      glhckObject* obj;
    };

    static kmVec3 const _xBase;
    static kmVec3 const _yBase;
    static kmVec3 const _zBase;
    static kmVec3 hexToRect(kmVec3 const& v);

    void clear();
    void initializeFromGame();

    glhckObject* createUnitObject(Game::Unit const& unit);
    glhckObject* createTileObject(Game::Tile const& tile);

    Game* _game;
    GLFWwindow* _window;
    glhckCamera* _camera;


    std::unordered_map<std::string, Unit> _units;
    std::unordered_map<std::string, Tile> _tiles;

    bool _shouldQuit;
  };
}
#endif // WARS_GLHCKVIEW_H
