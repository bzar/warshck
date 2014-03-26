#ifndef WARS_GLHCKVIEW_H
#define WARS_GLHCKVIEW_H

#include "view.h"
#include "glhck/glhck.h"
#include "GLFW/glfw3.h"
#include "gamenodepp.h"
#include "glfwhck.h"
#include "jsonpp.h"

#include <string>
#include <unordered_map>

namespace wars
{
  class GlhckView : public View
  {
  public:
    static void init(int argc, char** argv);
    static void term();

    GlhckView(Gamenode* gn);
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
      glhckObject* hex;
      glhckObject* prop;
    };
    struct InputState
    {
      struct
      {
        bool left = false;
        bool right = false;
        bool forward = false;
        bool backward = false;
        bool zoomIn = false;
        bool zoomOut = false;
      } camera;

      kmRay3 mouseRay;
      struct
      {
        int x = 0;
        int y = 0;
      } hexCursor;

      InputState() : mouseRay({{0,0,0}, {0,0,0}}) {}
    };

    struct Theme
    {
      struct Tile
      {
        struct Prop
        {
          std::string model;
          std::vector<std::string> textures;
        };

        std::string model;
        Prop prop;
        kmVec3 offset;
      };

      struct
      {
        kmVec3 x;
        kmVec3 y;
        kmVec3 z;
      } base;

      std::vector<glhckColorb> playerColors;
      std::vector<Tile> tiles;
    };

    kmVec3 hexToRect(kmVec3 const& v);
    kmVec3 rectToHex(kmVec3 const& v);

    void clear();
    void initializeFromGame();
    void handleInput();

    glhckObject* createUnitObject(Game::Unit const& unit);
    glhckObject* createTileHex(Game::Tile const& tile);
    glhckObject* createTileProp(Game::Tile const& tile);

    void loadTheme(std::string const& themeFile);

    Gamenode* _gn;
    Game* _game;
    GLFWwindow* _window;
    glhckCamera* _camera;
    glfwhckEventQueue* _glfwEvents;

    Theme _theme;

    std::unordered_map<std::string, Unit> _units;
    std::unordered_map<std::string, Tile> _tiles;

    bool _shouldQuit;
    InputState _inputState;
  };
}
#endif // WARS_GLHCKVIEW_H
