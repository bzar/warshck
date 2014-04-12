#ifndef WARS_GLHCKVIEW_H
#define WARS_GLHCKVIEW_H

#include "view.h"
#include "glhck/glhck.h"
#include "GLFW/glfw3.h"
#include "input.h"
#include "glfwhck.h"
#include "jsonpp.h"
#include "textmenu.h"

#include <string>
#include <unordered_map>
#include <memory>

namespace wars
{
  class GlhckView : public View
  {
  public:
    static void init(int argc, char** argv);
    static void term();

    GlhckView(Input* input);
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

      struct
      {
        kmRay3 ray = {{0,0,0}, {0,0,0}};
        bool leftButton = false;
      } mouse;
      struct
      {
        int x = 0;
        int y = 0;
      } hexCursor;

      struct
      {
        std::string tileId = "";
        std::string unitId = "";
        std::string carrierId = "";
        int carriedIndex = 0;
      } selected;

      bool acceptInput = false;
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
    void handleClick();
    void handleKey(int key);
    void initializeActionMenu();
    void updateStatusText();
    void setStatusText(std::string const& str);

    Input::Path convertPath(Game::Path const& path) const;

    glhckObject* createUnitObject(Game::Unit const& unit);
    glhckObject* createTileHex(Game::Tile const& tile);
    glhckObject* createTileProp(Game::Tile const& tile);

    void loadTheme(std::string const& themeFile);

    Input* _input;
    Game* _game;
    Stream<wars::Game::Event>::Subscription eventSub;
    GLFWwindow* _window;
    glhckCamera* _camera;
    glfwhckEventQueue* _glfwEvents;

    Theme _theme;

    std::unordered_map<std::string, Unit> _units;
    std::unordered_map<std::string, Tile> _tiles;

    bool _shouldQuit;
    InputState _inputState;

    enum class Phase { WAIT, SELECT, MOVE, ACTION, ATTACK, UNLOAD_UNIT, UNLOAD_TILE, BUILD };
    Phase _phase = Phase::SELECT;
    enum Action : int { CANCEL = 0, WAIT, ATTACK, CAPTURE, DEPLOY, UNDEPLOY, LOAD, UNLOAD };
    TextMenu _menu;
    glhckText* _statusText;
    unsigned int _statusFont;
  };
}
#endif // WARS_GLHCKVIEW_H
