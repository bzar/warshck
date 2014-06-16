#include "glhckview.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <sstream>

namespace
{
  void errorCallback(int code, char const* message)
  {
    std::cerr << "GLFW ERROR: " << message << std::endl;
  }
}


void wars::GlhckView::init(int argc, char** argv)
{
  if (!glfwInit())
  {
    throw std::runtime_error("GLFW initialization error");
  }
  glfwSetErrorCallback(errorCallback);

  glhckCompileFeatures features;
  glhckGetCompileFeatures(&features);
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_SAMPLES, 4);
  if(features.render.glesv1 || features.render.glesv2) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
  }
  if(features.render.glesv2) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  }
  if(features.render.opengl) {
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
  }

  if(!glhckContextCreate(argc, argv))
  {
    throw std::runtime_error("Failed to create a GLhck context");
  }
}

void wars::GlhckView::term()
{
  glhckContextTerminate();
  glfwTerminate();
}

wars::GlhckView::GlhckView(Input* input) :
  _input(input), _game(nullptr), _gameScene(nullptr), _window(nullptr), _shouldQuit(false),  _menu(),
  _funds(0), _statusText(nullptr), _statusFont(0), _gameInitialized(false)
{
  _window = glfwCreateWindow(800, 480, "warshck", NULL, NULL);
  _glfwEvents = glfwhckEventQueueNew(_window, GLFWHCK_EVENTS_ALL);
  if(!_window)
  {
    throw std::runtime_error("Failed to create a GLFW window");
  }

  glfwMakeContextCurrent(_window);

  glfwSwapInterval(1);

  glhckLogColor(0);
  if(!glhckDisplayCreate(800, 480, GLHCK_RENDER_AUTO))
  {
    throw std::runtime_error("Failed to create a GLhck display");
  }

  glhckRenderClearColorb(64, 64, 64, 255);

  _camera = glhckCameraNew();
  glhckCameraProjection(_camera, GLHCK_PROJECTION_PERSPECTIVE);
  glhckObjectPositionf(glhckCameraGetObject(_camera), 30, -110, 50);
  glhckObjectRotationf(glhckCameraGetObject(_camera), 0, 0, 0);
  glhckCameraRange(_camera, 0.1f, 1000.0f);
  glhckCameraFov(_camera, 45);

  kmVec3 target = {0, 1, -1};
  kmVec3Add(&target, &target, glhckObjectGetPosition(glhckCameraGetObject(_camera)));
  glhckObjectTarget(glhckCameraGetObject(_camera), &target);

  glhckCameraUpdate(_camera);

  _theme = loadTheme("config/theme.json");
}

wars::GlhckView::~GlhckView()
{
  glfwhckEventQueueFree(_glfwEvents);
}

void wars::GlhckView::setGame(Game* game)
{
  _game = game;

  if(_gameScene)
    delete _gameScene;
  _gameScene = new GameScene(_game, &_theme);

  eventSub = _game->events().on([this](wars::Game::Event const& e) {
    switch(e.type)
    {
      case wars::Game::EventType::GAMEDATA:
      {
        updateFunds();
        _gameInitialized = true;
        _inputState.acceptInput = true;
        break;
      }
      case wars::Game::EventType::MOVE:
      {
        break;
      }
      case wars::Game::EventType::WAIT:
      {
        break;
      }
      case wars::Game::EventType::ATTACK:
      {
        break;
      }
      case wars::Game::EventType::COUNTERATTACK:
      {
        break;
      }
      case wars::Game::EventType::CAPTURE:
      {
        break;
      }
      case wars::Game::EventType::CAPTURED:
      {
        break;
      }
      case wars::Game::EventType::DEPLOY:
      {
        break;
      }
      case wars::Game::EventType::UNDEPLOY:
      {
        break;
      }
      case wars::Game::EventType::LOAD:
      {
        break;
      }
      case wars::Game::EventType::UNLOAD:
      {
        break;
      }
      case wars::Game::EventType::DESTROY:
      {
        break;
      }
      case wars::Game::EventType::REPAIR:
      {
        break;
      }
      case wars::Game::EventType::BUILD:
      {
        updateFunds();
        break;
      }
      case wars::Game::EventType::REGENERATE_CAPTURE_POINTS:
      {
        break;
      }
      case wars::Game::EventType::PRODUCE_FUNDS:
      {
        break;
      }
      case wars::Game::EventType::BEGIN_TURN:
      {
        updateFunds();
        break;
      }
      case wars::Game::EventType::END_TURN:
      {
        break;
      }
      case wars::Game::EventType::TURN_TIMEOUT:
      {
        break;
      }
      case wars::Game::EventType::FINISHED:
      {
        break;
      }
      case wars::Game::EventType::SURRENDER:
      {
        break;
      }
      default:
      {
        break;
      }
    }
  });
}

bool wars::GlhckView::handle()
{
  if(!_gameInitialized)
    return true;

  glfwPollEvents();

  handleInput();

  glhckCameraUpdate(_camera);

  if(_gameScene)
    _gameScene->render();

  _menu.render();

  updateStatusText();
  if(_statusText != nullptr)
  {
    glhckTextRender(_statusText);
  }

  glfwSwapBuffers(_window);

  return !_shouldQuit;
}

void wars::GlhckView::quit()
{
  _shouldQuit = true;
}

void wars::GlhckView::handleInput()
{
  if(!_inputState.acceptInput)
    return;

  while(!glfwhckEventQueueEmpty(_glfwEvents))
  {
    const glfwhckEvent* e = glfwhckEventQueuePop(_glfwEvents);
    switch(e->type)
    {
      case GLFWHCK_EVENT_MOUSE_POSITION:
      {
        int w, h;
        glfwGetWindowSize(_window, &w, &h);
        glhckCameraCastRayFromPointf(_camera, &_inputState.mouse.ray,
                                     e->mousePosition.x/w,
                                     1.0f - e->mousePosition.y/h);
        break;
      }
      case GLFWHCK_EVENT_MOUSE_BUTTON:
      {
        if(e->mouseButton.button == GLFW_MOUSE_BUTTON_LEFT)
        {
          _inputState.mouse.leftButton = e->mouseButton.action != GLFW_RELEASE;
        }
        break;
      }
      case GLFWHCK_EVENT_KEYBOARD_KEY:
      {
        switch(e->keyboardKey.key)
        {
          case GLFW_KEY_LEFT:
          {
            _inputState.camera.left = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_RIGHT:
          {
            _inputState.camera.right = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_UP:
          {
            _inputState.camera.forward = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_DOWN:
          {
            _inputState.camera.backward = e->keyboardKey.action != GLFW_RELEASE;
            break;

          }
          case GLFW_KEY_PAGE_UP:
          {
            _inputState.camera.zoomIn = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_PAGE_DOWN:
          {
            _inputState.camera.zoomOut = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_SPACE:
          {
            if(_phase == Phase::SELECT)
            {
              _phase = Phase::GAME_MENU;
              _menu.clear();
              _menu.addOption(GameMenuAction::CANCEL, "Cancel");
              if(_game->getInTurn().isMe)
              {
                _menu.addOption(GameMenuAction::END_TURN, "End turn");
                _menu.addOption(GameMenuAction::SURRENDER, "Surrender");
              }
              _menu.addOption(GameMenuAction::QUIT, "Quit");
              _menu.update();
            }
            break;
          }
          case GLFW_KEY_ESCAPE:
          {
            quit();
            break;
          }
          default:
          {
            if(e->keyboardKey.action == GLFW_PRESS)
            {
              handleKey(e->keyboardKey.key);
            }
          }
        }
        break;
      }
      case GLFWHCK_EVENT_WINDOW_CLOSE:
      {
        quit();
        break;
      }
      case GLFWHCK_EVENT_WINDOW_RESIZE:
      {
        if(e->windowResize.width > 0 && e->windowResize.height > 0)
          glhckDisplayResize(e->windowResize.width, e->windowResize.height);
        break;
      }
      default:
        break;
    }
  }

  if(_inputState.camera.left)
  {
    glhckObjectMovef(glhckCameraGetObject(_camera), -0.5, 0, 0);
  }

  if(_inputState.camera.right)
  {
    glhckObjectMovef(glhckCameraGetObject(_camera), 0.5, 0, 0);
  }

  if(_inputState.camera.forward)
  {
    glhckObjectMovef(glhckCameraGetObject(_camera), 0, 0.5, 0);
  }
  if(_inputState.camera.backward)
  {
    glhckObjectMovef(glhckCameraGetObject(_camera), 0, -0.5, 0);
  }

  if(_inputState.camera.zoomIn
     && glhckObjectGetPosition(glhckCameraGetObject(_camera))->z > 10)
  {
    glhckObjectMovef(glhckCameraGetObject(_camera), 0, 0.5, -0.5);
  }

  if(_inputState.camera.zoomOut)
  {
    glhckObjectMovef(glhckCameraGetObject(_camera), 0, -0.5, 0.5);
  }

  if(_inputState.camera.left
     || _inputState.camera.right
     || _inputState.camera.forward
     || _inputState.camera.backward
     || _inputState.camera.zoomIn
     || _inputState.camera.zoomOut)
  {
    kmVec3 target = {0, 1, -1};
    kmVec3Add(&target, &target, glhckObjectGetPosition(glhckCameraGetObject(_camera)));
    glhckObjectTarget(glhckCameraGetObject(_camera), &target);
  }

  if(_inputState.mouse.ray.dir.z)
  {
    kmVec3 pointer;
    kmPlane plane = {0, 0, 1, 0};
    kmRay3IntersectPlane(&pointer, &_inputState.mouse.ray, &plane);

    pointer = _gameScene->rectToHex(pointer);
    _inputState.hexCursor.x = static_cast<int>(std::round(pointer.x));
    _inputState.hexCursor.y = static_cast<int>(std::round(pointer.y));
  }

  if(_inputState.mouse.leftButton)
  {
    handleClick();
    _inputState.mouse.leftButton = false;
  }
}

void wars::GlhckView::handleClick()
{
  Game::Player const& inTurn = _game->getInTurn();
  switch (_phase)
  {
    case Phase::SELECT:
    {
      if(inTurn.isMe)
      {
        Game::Tile const* tile = _game->getTileAt(_inputState.hexCursor.x,
                                                  _inputState.hexCursor.y);
        Rules const& rules = _game->getRules();

        if(tile != nullptr)
        {
          if(tile->unitId.empty())
          {
            TerrainType const& terrain = rules.terrainTypes.at(tile->type);
            if(tile->owner == inTurn.playerNumber
               && !terrain.buildTypes.empty())
            {
              _inputState.selected.tileId = tile->id;
              _phase = Phase::BUILD;

              _menu.clear();
              for(auto const& item : rules.unitTypes)
              {
                UnitType const& t = item.second;
                if(terrain.buildTypes.find(t.unitClass) != terrain.buildTypes.end())
                {
                  std::ostringstream oss;
                  oss << t.name << " (" << t.price << ")";
                  _menu.addOption(t.id, oss.str(), t.id);
                }
              }
              _menu.update();
            }
          }
          else
          {
            Game::Unit const& unit = _game->getUnit(tile->unitId);
            if(unit.owner == inTurn.playerNumber && !unit.moved)
            {
              _inputState.selected.unitId = unit.id;
              _inputState.hexOptions = _game->findMovementOptions(unit.id);
              if(unit.deployed || _inputState.hexOptions.size() <= 1)
              {
                _phase = Phase::ACTION;
                _inputState.selected.tileId = tile->id;
                initializeActionMenu();

              }
              else
              {
                _phase = Phase::MOVE;
                _gameScene->setHighlightedTiles(_inputState.hexOptions);
              }
            }
          }
        }
      }
      break;
    }

    case Phase::ACTION:
    {
     break;
    }

    case Phase::MOVE:
    {
      _gameScene->clearHighlightedTiles();

      Game::Coordinates coords = {_inputState.hexCursor.x, _inputState.hexCursor.y};
      if(std::find(_inputState.hexOptions.begin(), _inputState.hexOptions.end(), coords) != _inputState.hexOptions.end())
      {
        _inputState.selected.tileId = _game->getTileAt(_inputState.hexCursor.x, _inputState.hexCursor.y)->id;
        _phase = Phase::ACTION;
        initializeActionMenu();
      }
      else
      {
        _phase = Phase::SELECT;
      }
      break;
    }

    case Phase::ATTACK:
    {
      Game::Tile const* enemyTile = _game->getTileAt(_inputState.hexCursor.x, _inputState.hexCursor.y);
      if(enemyTile->unitId.empty() || _inputState.attackOptions.find(enemyTile->unitId) == _inputState.attackOptions.end())
      {
        _phase = Phase::SELECT;
      }
      else
      {
        Game::Unit const& enemyUnit = _game->getUnit(enemyTile->unitId);
        Game::Tile const& tile = _game->getTile(_inputState.selected.tileId);
        Game::Path path = _game->findUnitPath(_inputState.selected.unitId, {tile.x, tile.y});
        if(path.empty())
        {
          // Cannot move to location
          _phase = Phase::SELECT;
        }
        else
        {
          _phase = Phase::WAIT;
          _input->moveAttack(_game->getGameId(), _inputState.selected.unitId, enemyUnit.id, {tile.x, tile.y}, convertPath(path)).then<void>([this](bool success) {
            std::cout << "moveAttack " << (success ? "SUCCESS" : "FAILURE") << std::endl;
            _phase = Phase::SELECT;
          });
        }
      }

      _gameScene->clearAttackOptions(_inputState.attackOptions);
      break;
    }

    case Phase::UNLOAD_UNIT:
    {
      _phase = Phase::SELECT;
      break;
    }
    case Phase::UNLOAD_TILE:
    {
      _gameScene->clearHighlightedTiles();

      Game::Unit const& unit = _game->getUnit(_inputState.selected.unitId);
      std::string const& carriedId = unit.carriedUnits.at(_inputState.selected.carriedIndex);
      std::string const& tileId = _inputState.selected.tileId;
      Game::Coordinates destination = {_inputState.hexCursor.x, _inputState.hexCursor.y};

      if(_game->unitCanUnloadUnitFromTileToCoordinates(unit.id, carriedId, tileId, destination))
      {
        Game::Tile const& unloadTile = _game->getTile(tileId);
        Game::Path path = _game->findUnitPath(unit.id, {unloadTile.x, unloadTile.y});
        if(path.empty())
        {
          _phase = Phase::SELECT;
        }
        else
        {
          Input::Position unloadPosition = {unloadTile.x, unloadTile.y};
          Input::Position destinationPosition = {destination.x, destination.y};

          _phase = Phase::WAIT;
          _input->moveUnload(_game->getGameId(), unit.id, unloadPosition, convertPath(path), carriedId, destinationPosition).then<void>([this](bool success) {
            std::cout << "moveUnload " << (success ? "SUCCESS" : "FAILURE") << std::endl;
            _phase = Phase::SELECT;
          });
        }
      }
      else
      {
        _phase = Phase::SELECT;
      }
      break;
    }
    case Phase::BUILD:
    {
      _phase = Phase::SELECT;
      _menu.clear();
      _menu.update();
      break;
    }

    case Phase::GAME_MENU:
    {
      _phase = Phase::SELECT;
      _menu.clear();
      _menu.update();
    }

    default:
      break;
  }
}

void wars::GlhckView::handleKey(int key)
{
  Game::Player const& inTurn = _game->getInTurn();
  switch(_phase)
  {
    case Phase::SELECT:
    {
      break;
    }

    case Phase::ACTION:
    {
      int result;
      if(_menu.input(key, &result))
      {
        _menu.clear();
        Action action = static_cast<Action>(result);
        switch(action) {
          case Action::CANCEL:
          {
            _phase = Phase::SELECT;
            break;
          }
          case Action::WAIT:
          {
            Game::Tile const& tile = _game->getTile(_inputState.selected.tileId);
            Game::Path path = _game->findUnitPath(_inputState.selected.unitId, {tile.x, tile.y});
            if(path.empty())
            {
              // Cannot move to location
              _phase = Phase::SELECT;
            }
            else
            {
              _phase = Phase::WAIT;
              _input->moveWait(_game->getGameId(), _inputState.selected.unitId, {tile.x, tile.y}, convertPath(path)).then<void>([this](bool success) {
                std::cout << "moveWait " << (success ? "SUCCESS" : "FAILURE") << std::endl;
                _phase = Phase::SELECT;
              });
            }

            break;
          }
          case Action::ATTACK:
          {
            _phase = Phase::ATTACK;
            Game::Tile const& tile = _game->getTile(_inputState.selected.tileId);
            _inputState.attackOptions = _game->findAttackOptions(_inputState.selected.unitId, {tile.x, tile.y});
            std::cout << "Attack options:" << std::endl;
            for(auto const& o : _inputState.attackOptions)
            {
              std::cout << o.first << ": " << o.second << std::endl;
            }

            _gameScene->setAttackOptions(_inputState.attackOptions);
            break;
          }
          case Action::CAPTURE:
          {
            Game::Tile const& tile = _game->getTile(_inputState.selected.tileId);
            Game::Path path = _game->findUnitPath(_inputState.selected.unitId, {tile.x, tile.y});
            if(path.empty())
            {
              // Cannot move to location
              _phase = Phase::SELECT;
            }
            else
            {
              _phase = Phase::WAIT;
              _input->moveCapture(_game->getGameId(), _inputState.selected.unitId, {tile.x, tile.y}, convertPath(path)).then<void>([this](bool success) {
                std::cout << "moveCapture " << (success ? "SUCCESS" : "FAILURE") << std::endl;
                _phase = Phase::SELECT;
              });
            }
            break;
          }
          case Action::DEPLOY:
          {
            Game::Tile const& tile = _game->getTile(_inputState.selected.tileId);
            Game::Path path = _game->findUnitPath(_inputState.selected.unitId, {tile.x, tile.y});
            if(path.empty())
            {
              // Cannot move to location
              _phase = Phase::SELECT;
            }
            else
            {
              _phase = Phase::WAIT;
              _input->moveDeploy(_game->getGameId(), _inputState.selected.unitId, {tile.x, tile.y}, convertPath(path)).then<void>([this](bool success) {
                std::cout << "moveDeploy " << (success ? "SUCCESS" : "FAILURE") << std::endl;
                _phase = Phase::SELECT;
              });
            }
            break;
          }
          case Action::UNDEPLOY:
          {
            _phase = Phase::WAIT;
            _input->undeploy(_game->getGameId(), _inputState.selected.unitId).then<void>([this](bool success) {
              std::cout << "undeploy " << (success ? "SUCCESS" : "FAILURE") << std::endl;
              _phase = Phase::SELECT;
            });
            break;
          }
          case Action::LOAD:
          {
            Game::Tile const& tile = _game->getTile(_inputState.selected.tileId);
            if(tile.unitId.empty() || !_game->unitCanLoadInto(_inputState.selected.unitId, tile.unitId))
            {
              // No unit to load into
              _phase = Phase::SELECT;
            }
            else
            {
              Game::Path path = _game->findUnitPath(_inputState.selected.unitId, {tile.x, tile.y});
              if(path.empty())
              {
                // Cannot move to location
                _phase = Phase::SELECT;
              }
              else
              {
                _phase = Phase::WAIT;
                _input->moveLoad(_game->getGameId(), _inputState.selected.unitId, tile.unitId, convertPath(path)).then<void>([this](bool success) {
                  std::cout << "moveDeploy " << (success ? "SUCCESS" : "FAILURE") << std::endl;
                  _phase = Phase::SELECT;
                });
              }
            }

            break;
          }
          case Action::UNLOAD:
          {
            Game::Unit const& unit = _game->getUnit(_inputState.selected.unitId);
            if(unit.carriedUnits.empty())
            {
              _phase = Phase::SELECT;
            }
            else
            {
              _phase = Phase::UNLOAD_UNIT;
              for(int i = 0; i < unit.carriedUnits.size(); ++i)
              {
                Game::Unit const& carried = _game->getUnit(unit.carriedUnits.at(i));
                UnitType const& carriedType = _game->getRules().unitTypes.at(carried.type);
                std::ostringstream oss;
                oss << carriedType.name << " (" << unit.health << ")";
                _menu.addOption(i, oss.str(), i);
              }
            }
            break;
          }
          default:
            break;
        }
      }
      _menu.update();
      break;
    }

    case Phase::MOVE:
    {
      break;
    }
    case Phase::ATTACK:
    {
      break;
    }
    case Phase::UNLOAD_UNIT:
    {
      int result;
      if(_menu.input(key, &result))
      {
        _inputState.selected.carriedIndex = result;
        _phase = Phase::UNLOAD_TILE;
        _menu.clear();
        _menu.update();

        Game::Unit const& carrier = _game->getUnit(_inputState.selected.unitId);
        Game::Unit const& carried = _game->getUnit(carrier.carriedUnits.at(_inputState.selected.carriedIndex));
        _inputState.hexOptions = _game->unitUnloadUnitFromTileOptions(carrier.id, carried.id, _inputState.selected.tileId);

        _gameScene->setHighlightedTiles(_inputState.hexOptions);
      }
      break;
    }
    case Phase::UNLOAD_TILE:
    {
      break;
    }
    case Phase::BUILD:
    {
      int result;
      if(_menu.input(key, &result))
      {
        UnitType const& unitType = _game->getRules().unitTypes.at(result);
        if(_funds >= unitType.price)
        {
          std::cout << "Build unit id " << result << std::endl;
          Game::Tile const& tile = _game->getTile(_inputState.selected.tileId);
          _input->build(_game->getGameId(), {tile.x, tile.y}, result).then<void>([this](bool const& success) {
            _phase = Phase::SELECT;
            std::cout << "Build " << (success ? "SUCCESS" : "FAILURE") << std::endl;
          });
          _phase = Phase::WAIT;
          _menu.clear();
          _menu.update();
        }
      }
      break;
    }

    case Phase::GAME_MENU:
    {
      int result;
      if(_menu.input(key, &result))
      {
        _menu.clear();
        GameMenuAction action = static_cast<GameMenuAction>(result);
        switch(action)
        {
          case GameMenuAction::CANCEL:
          {
            _phase = Phase::SELECT;
            break;
          }
          case GameMenuAction::END_TURN:
          {
            _phase = Phase::WAIT;
            _input->endTurn(_game->getGameId()).then<void>([this](bool const& success) {
              _phase = Phase::SELECT;
              std::cout << "End turn " << (success ? "SUCCESS" : "FAILURE") << std::endl;
            });
            break;
          }
          case GameMenuAction::SURRENDER:
          {
            _phase = Phase::WAIT;
            _input->surrender(_game->getGameId()).then<void>([this](bool const& success) {
              _phase = Phase::SELECT;
              std::cout << "Surrender " << (success ? "SUCCESS" : "FAILURE") << std::endl;
            });
            break;
          }
          case GameMenuAction::QUIT:
          {
            quit();
            break;
          }
        }
        _menu.update();
      }
      break;
    }

    default:
      break;
  }
}

void wars::GlhckView::initializeActionMenu()
{
  Game::Tile const& tile = _game->getTile(_inputState.selected.tileId);

  _menu.clear();
  _menu.addOption(Action::CANCEL, "Cancel");
  if(!_game->unitCanLoadInto(_inputState.selected.unitId, tile.unitId))
    _menu.addOption(Action::WAIT, "Wait");
  if(_game->unitCanAttackFromTile(_inputState.selected.unitId, _inputState.selected.tileId))
    _menu.addOption(Action::ATTACK, "Attack");
  if(_game->unitCanCaptureTile(_inputState.selected.unitId, _inputState.selected.tileId))
    _menu.addOption(Action::CAPTURE, "Capture");
  if(_game->unitCanDeployAtTile(_inputState.selected.unitId, _inputState.selected.tileId))
    _menu.addOption(Action::DEPLOY, "Deploy");
  if(_game->unitCanUndeploy(_inputState.selected.unitId, _inputState.selected.tileId))
    _menu.addOption(Action::UNDEPLOY, "Undeploy");
  if(_game->unitCanLoadInto(_inputState.selected.unitId, tile.unitId))
    _menu.addOption(Action::LOAD, "Load");
  if(_game->unitCanUnloadAtTile(_inputState.selected.unitId, _inputState.selected.tileId))
    _menu.addOption(Action::UNLOAD, "Unload");
  _menu.update();
}

void wars::GlhckView::updateStatusText()
{
  static std::string const PHASE_NAMES[] = {
    "WAIT",
    "SELECT",
    "MOVE",
    "ACTION",
    "ATTACK",
    "UNLOAD_UNIT",
    "UNLOAD_TILE",
    "BUILD",
    "GAME_MENU"
  };

  std::ostringstream oss;
  oss << PHASE_NAMES[static_cast<int>(_phase)]
      << " | " << _funds << " credits";
  oss << " | " << "Player " << _game->getInTurn().playerNumber << " (" << _game->getInTurn().playerName << ")";
  setStatusText(oss.str());
}

void wars::GlhckView::setStatusText(const std::string& str)
{
  if(_statusText == nullptr)
  {
    _statusText = glhckTextNew(512, 512);
    int nativeSize;
    _statusFont = glhckTextFontNewKakwafont(_statusText, &nativeSize);
  }
  int w, h;
  glfwGetWindowSize(_window, &w, &h);

  glhckTextClear(_statusText);
  glhckTextStash(_statusText, _statusFont, 24, 4, h, str.data(), nullptr);
}

void wars::GlhckView::updateFunds()
{
  _input->funds(_game->getGameId()).then<void>([this](int const& value) {
    _funds = value;
  });
}

wars::Input::Path wars::GlhckView::convertPath(const wars::Game::Path& path) const
{
  Input::Path result;
  for(Game::Coordinates const& c : path)
  {
    Input::Position p = {c.x, c.y};
    result.push_back(p);
  }
  return result;
}
