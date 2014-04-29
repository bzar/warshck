#ifndef WARS_GLHCKVIEW_H
#define WARS_GLHCKVIEW_H

#include "view.h"
#include "glhck/glhck.h"
#include "GLFW/glfw3.h"
#include "input.h"
#include "glfwhck.h"
#include "jsonpp.h"
#include "textmenu.h"
#include "hexlabel.h"
#include "theme.h"

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
      std::vector<Game::Coordinates> hexOptions;
      std::unordered_map<std::string, int> attackOptions;
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
    void updateFunds();
    void updatePropTexture(glhckObject* o, int terrainId, int owner);
    void updateHexLabel(std::string const& id);

    Input::Path convertPath(Game::Path const& path) const;

    glhckObject* createUnitObject(Game::Unit const& unit);
    glhckObject* createTileHex(Game::Tile const& tile);
    glhckObject* createTileProp(Game::Tile const& tile);

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

    enum class Phase : int { WAIT = 0, SELECT, MOVE, ACTION, ATTACK, UNLOAD_UNIT, UNLOAD_TILE, BUILD, GAME_MENU };
    Phase _phase = Phase::SELECT;
    enum class Action : int { CANCEL = 0, WAIT, ATTACK, CAPTURE, DEPLOY, UNDEPLOY, LOAD, UNLOAD };
    enum class GameMenuAction : int { CANCEL = 0, END_TURN, SURRENDER, QUIT };

    TextMenu _menu;

    int _funds;
    glhckText* _statusText;
    unsigned int _statusFont;

    glhckObject* _sky;
    bool _gameInitialized;
  };
}
#endif // WARS_GLHCKVIEW_H
