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
  _input(input), _window(nullptr), _shouldQuit(false), _units(), _tiles(), _menu(),
  _funds(0), _statusText(nullptr), _statusFont(0), _sky(nullptr)
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

  loadTheme("config/theme.json");
}

wars::GlhckView::~GlhckView()
{
  glfwhckEventQueueFree(_glfwEvents);
}

void wars::GlhckView::setGame(Game* game)
{
  _game = game;

  eventSub = _game->events().on([this](wars::Game::Event const& e) {
    switch(e.type)
    {
      case wars::Game::EventType::GAMEDATA:
      {
        initializeFromGame();
        updateFunds();
        break;
      }
      case wars::Game::EventType::MOVE:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.move.unitId);
        wars::Game::Tile const& next = _game->getTile(*e.move.tileId);
        wars::Game::Tile const& prev = _game->getTile(unit.tileId);
        glhckObject* o = _units.at(unit.id).obj;
        kmVec3 pos = hexToRect({static_cast<kmScalar>(next.x), static_cast<kmScalar>(next.y), 1});
        glhckObjectPositionf(o, pos.x, pos.y, pos.z);
        break;
      }
      case wars::Game::EventType::WAIT:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.wait.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        break;
      }
      case wars::Game::EventType::ATTACK:
      {
        wars::Game::Unit const& attacker = _game->getUnit(*e.attack.attackerId);
        wars::Game::Unit const& target = _game->getUnit(*e.attack.targetId);
        break;
      }
      case wars::Game::EventType::COUNTERATTACK:
      {
        wars::Game::Unit const& attacker = _game->getUnit(*e.counterattack.attackerId);
        wars::Game::Unit const& target = _game->getUnit(*e.counterattack.targetId);
        break;
      }
      case wars::Game::EventType::CAPTURE:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.capture.unitId);
        wars::Game::Tile const& tile = _game->getTile(*e.capture.tileId);
        break;
      }
      case wars::Game::EventType::CAPTURED:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.captured.unitId);
        wars::Game::Tile const& tile = _game->getTile(*e.captured.tileId);
        break;
      }
      case wars::Game::EventType::DEPLOY:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.deploy.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        break;
      }
      case wars::Game::EventType::UNDEPLOY:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.undeploy.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        break;
      }
      case wars::Game::EventType::LOAD:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.load.unitId);
        wars::Game::Unit const& carrier = _game->getUnit(*e.load.carrierId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        glhckObject* o = _units.at(unit.id).obj;
        _units.erase(unit.id);
        glhckObjectFree(o);
        break;
      }
      case wars::Game::EventType::UNLOAD:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.unload.unitId);
        wars::Game::Unit const& carrier = _game->getUnit(*e.unload.carrierId);
        wars::Game::Tile const& next = _game->getTile(*e.unload.tileId);
        glhckObject* unitObject = createUnitObject(unit);
        _units[unit.id] = {unit.id, unitObject};
        kmVec3 pos = hexToRect({static_cast<kmScalar>(next.x), static_cast<kmScalar>(next.y), 1});
        glhckObjectPositionf(unitObject, pos.x, pos.y, pos.z);
        break;
      }
      case wars::Game::EventType::DESTROY:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.destroy.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        glhckObject* o = _units.at(unit.id).obj;
        _units.erase(unit.id);
        glhckObjectFree(o);
        break;
      }
      case wars::Game::EventType::REPAIR:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.repair.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        break;
      }
      case wars::Game::EventType::BUILD:
      {
        updateFunds();
        wars::Game::Unit const& unit = _game->getUnit(*e.build.unitId);
        wars::Game::Tile const& tile = _game->getTile(*e.build.tileId);
        glhckObject* unitObject = createUnitObject(unit);
        if(unitObject != nullptr)
          _units[unit.id] = {unit.id, unitObject};
        break;
      }
      case wars::Game::EventType::REGENERATE_CAPTURE_POINTS:
      {
        wars::Game::Tile const& tile = _game->getTile(*e.regenerateCapturePoints.tileId);
        break;
      }
      case wars::Game::EventType::PRODUCE_FUNDS:
      {
        wars::Game::Tile const& tile = _game->getTile(*e.produceFunds.tileId);
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
  glfwPollEvents();

  handleInput();

  glhckCameraUpdate(_camera);

  if(_sky != nullptr)
    glhckObjectDraw(_sky);

  bool tilesHighlighted = false;
  for(auto& item : _tiles)
  {
    Game::Tile const& tile = _game->getTile(item.first);

    bool selected = tile.x == _inputState.hexCursor.x
        && tile.y == _inputState.hexCursor.y;
    glhckObjectDrawAABB(item.second.hex, selected);
    glhckObjectDraw(item.second.hex);

    tilesHighlighted |= item.second.effects.highlight;

    if(item.second.prop != nullptr)
      glhckObjectDraw(item.second.prop);

  }

  bool unitsHighlighted = false;
  for(auto& item : _units)
  {
    unitsHighlighted |= item.second.effects.highlight;
    glhckObjectDraw(item.second.obj);
  }

  glhckRenderClear(GLHCK_DEPTH_BUFFER_BIT | GLHCK_COLOR_BUFFER_BIT);
  glhckRender();

  if(tilesHighlighted)
  {
    glhckRenderBlendFunc(GLHCK_ONE, GLHCK_ONE);
    for(auto& item : _tiles)
    {
      if(item.second.effects.highlight)
      {
        glhckObjectDraw(item.second.hex);
        if(item.second.prop != nullptr)
          glhckObjectDraw(item.second.prop);
      }
    }

    glhckRender();
    glhckRenderBlendFunc(GLHCK_ZERO, GLHCK_ZERO);
  }

  if(unitsHighlighted)
  {
    glhckRenderBlendFunc(GLHCK_ONE, GLHCK_ONE);
    for(auto& item : _units)
    {
      if(item.second.effects.highlight)
      {
        glhckObjectDraw(item.second.obj);
      }
    }

    glhckRender();
    glhckRenderBlendFunc(GLHCK_ZERO, GLHCK_ZERO);
  }

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

kmVec3 wars::GlhckView::hexToRect(const kmVec3& v)
{
  kmVec3 result, r;
  kmVec3Scale(&result, &_theme.base.x, v.x);
  kmVec3Scale(&r, &_theme.base.y, v.y);
  kmVec3Add(&result, &result, &r);
  kmVec3Scale(&r, &_theme.base.z, v.z);
  kmVec3Add(&result, &result, &r);
  return result;
}

kmVec3 wars::GlhckView::rectToHex(const kmVec3& v)
{

  kmVec3 result;
  kmMat4 mat = {
    _theme.base.x.x, _theme.base.x.y, _theme.base.x.z, 0,
    _theme.base.y.x, _theme.base.y.y, _theme.base.y.z, 0,
    _theme.base.z.x, _theme.base.z.y, _theme.base.z.z, 0,
    0, 0, 0, 1
  };

  kmMat4Inverse(&mat, &mat);

  kmVec3Transform(&result, &v, &mat);
  return result;
}

void wars::GlhckView::clear()
{
  for(auto& item : _units)
  {
    glhckObjectFree(item.second.obj);
  }
  _units.clear();

  for(auto& item : _tiles)
  {
    glhckObjectFree(item.second.hex);
    if(item.second.prop != nullptr)
      glhckObjectFree(item.second.prop);
  }
  _tiles.clear();

  if(_sky != nullptr)
    glhckObjectFree(_sky);
}

void wars::GlhckView::initializeFromGame()
{
  clear();

  std::unordered_map<std::string, Game::Tile> const& tiles = _game->getTiles();
  std::unordered_map<std::string, Game::Unit> const& units = _game->getUnits();
  Rules const& rules = _game->getRules();

  bool first = true;
  kmVec3 minCoords = {0, 0, 0};
  kmVec3 maxCoords = {0, 0, 0};
  for(auto item : tiles)
  {
    minCoords.x = std::min(minCoords.x, static_cast<float>(item.second.x));
    minCoords.y = std::min(minCoords.y, static_cast<float>(item.second.y));
    maxCoords.x = std::max(maxCoords.x, static_cast<float>(item.second.x));
    maxCoords.y = std::max(maxCoords.y, static_cast<float>(item.second.y));

    _tiles[item.first] = {
      item.first,
      createTileHex(item.second),
      createTileProp(item.second)
    };
    first = false;
  }

  kmVec3 minWorldCoords = hexToRect(minCoords);
  kmVec3 maxWorldCoords = hexToRect(maxCoords);

  float skySphereSize = std::max(maxWorldCoords.x - minCoords.x, maxWorldCoords.y - minWorldCoords.y);
  kmVec3 skySpherePosition;
  kmVec3Add(&skySpherePosition, &minWorldCoords, &maxWorldCoords);
  kmVec3Scale(&skySpherePosition, &skySpherePosition, 0.5);

  _sky = glhckModelNew("models/sky.glhckm", skySphereSize, glhckImportDefaultModelParameters());
  glhckObjectPosition(_sky, &skySpherePosition);

  for(auto item : units)
  {
    if(!item.second.tileId.empty())
    {
      glhckObject* unitObject = createUnitObject(item.second);
      _units[item.first] = {item.first, unitObject};
    }
  }

  _inputState.acceptInput = true;
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
          case GLFW_KEY_A:
          {
            _inputState.camera.left = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_RIGHT:
          case GLFW_KEY_D:
          {
            _inputState.camera.right = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_UP:
          case GLFW_KEY_W:
          {
            _inputState.camera.forward = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_DOWN:
          case GLFW_KEY_S:
          {
            _inputState.camera.backward = e->keyboardKey.action != GLFW_RELEASE;
            break;

          }
          case GLFW_KEY_R:
          {
            _inputState.camera.zoomIn = e->keyboardKey.action != GLFW_RELEASE;
            break;
          }
          case GLFW_KEY_F:
          {
            _inputState.camera.zoomOut = e->keyboardKey.action != GLFW_RELEASE;
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

    pointer = rectToHex(pointer);
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
                  _menu.addOption(t.id, t.name, t.id);
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

                for(auto& item : _tiles)
                {
                  Game::Tile const& t = _game->getTile(item.first);
                  Game::Coordinates c = {t.x, t.y};
                  item.second.effects.highlight = std::find(_inputState.hexOptions.begin(), _inputState.hexOptions.end(), c) != _inputState.hexOptions.end();
                }
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
      for(auto& item : _tiles)
      {
        item.second.effects.highlight = false;
      }

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

      for(auto& item : _units)
      {
        item.second.effects.highlight = false;
      }

      break;
    }

    case Phase::UNLOAD_UNIT:
    {
      _phase = Phase::SELECT;
      break;
    }
    case Phase::UNLOAD_TILE:
    {
      break;
    }
    case Phase::BUILD:
    {
      _phase = Phase::SELECT;
      _menu.clear();
      _menu.update();
      break;
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
              std::cout << o.first << ": " << o.second << std::endl;

            for(auto& item : _units)
            {
              item.second.effects.highlight = _inputState.attackOptions.find(item.first) != _inputState.attackOptions.end();
            }

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
            Game::Path path = _game->findUnitPath(_inputState.selected.unitId, {tile.x, tile.y});
            if(path.empty())
            {
              // Cannot move to location
              _phase = Phase::SELECT;
            }
            else
            {
              _phase = Phase::WAIT;
              _input->moveLoad(_game->getGameId(), _inputState.selected.unitId, _inputState.selected.carrierId, convertPath(path)).then<void>([this](bool success) {
                std::cout << "moveDeploy " << (success ? "SUCCESS" : "FAILURE") << std::endl;
                _phase = Phase::SELECT;
              });
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
              _menu.clear();
              for(int i = 0; i < unit.carriedUnits.size(); ++i)
              {
                _menu.addOption(i, unit.carriedUnits.at(i), i);
              }
              _menu.update();
            }
            break;
          }
          default:
            break;
        }
      }
      _menu.clear();
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
      break;
    }


    default:
      break;
  }
}

void wars::GlhckView::initializeActionMenu()
{
  _menu.clear();
  _menu.addOption(Action::CANCEL, "Cancel");
  _menu.addOption(Action::WAIT, "Wait");
  _menu.addOption(Action::ATTACK, "Attack");
  _menu.addOption(Action::CAPTURE, "Capture");
  _menu.addOption(Action::DEPLOY, "Deploy");
  _menu.addOption(Action::UNDEPLOY, "Undeploy");
  _menu.addOption(Action::LOAD, "Load");
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
    "BUILD"
  };

  std::ostringstream oss;
  oss << PHASE_NAMES[static_cast<int>(_phase)] << " | " << _funds << " credits";
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

glhckObject* wars::GlhckView::createUnitObject(const wars::Game::Unit& unit)
{
  glhckObject* o = glhckCubeNew(1.0);

  if(!unit.tileId.empty())
  {
    Game::Tile const& tile = _game->getTile(unit.tileId);
    kmVec3 pos = hexToRect({static_cast<kmScalar>(tile.x), static_cast<kmScalar>(tile.y), 1.0f});
    glhckObjectPositionf(o, pos.x, pos.y, pos.z);
  }
  std::string const files[] = {
    "textures/inf_placeholder.png",
    "textures/at_placeholder.png",
    "textures/sc_placeholder.png",
    "textures/lt_placeholder.png",
    "textures/mt_placeholder.png",
    "textures/ht_placeholder.png",
    "textures/la_placeholder.png",
    "textures/ma_placeholder.png",
    "textures/ha_placeholder.png",
    "textures/aa_placeholder.png",
    "textures/sm_placeholder.png",
    "textures/co_placeholder.png",
    "textures/in_placeholder.png",
    "textures/bo_placeholder.png",
    "textures/ap_placeholder.png",
    "textures/tc_placeholder.png",
    "textures/cs_placeholder.png",
    "textures/gb_placeholder.png",
    "textures/ab_placeholder.png",
    "textures/cr_placeholder.png"
  };
  glhckTexture* t = glhckTextureNewFromFile(files[unit.type].data(), glhckImportDefaultImageParameters(), glhckTextureDefaultSpriteParameters());
  glhckMaterial* m = glhckMaterialNew(t);
  glhckTextureFree(t);
  glhckMaterialDiffuse(m, &_theme.playerColors[unit.owner]);
  glhckObjectMaterial(o, m);
  glhckMaterialFree(m);
  glhckObjectDrawOBB(o, 1);
  return o;
}

glhckObject* wars::GlhckView::createTileHex(const wars::Game::Tile& tile)
{
  TerrainType const& terrain = _game->getRules().terrainTypes.at(tile.type);
  glhckObject* o = glhckModelNew(_theme.tiles[tile.type].model.data(), 1.0, glhckImportDefaultModelParameters());
  kmVec3 pos = {static_cast<kmScalar>(tile.x), static_cast<kmScalar>(tile.y), 0};
  kmVec3Add(&pos, &pos, &_theme.tiles[tile.type].offset);
  pos = hexToRect(pos);
  glhckObjectPositionf(o, pos.x, pos.y, pos.z);
  return o;
}

glhckObject*wars::GlhckView::createTileProp(const wars::Game::Tile& tile)
{
  TerrainType const& terrain = _game->getRules().terrainTypes.at(tile.type);
  if(_theme.tiles[terrain.id].prop.model.empty())
    return nullptr;

  glhckObject* o = glhckModelNew(_theme.tiles[terrain.id].prop.model.data(), 1.0, glhckImportDefaultModelParameters());

  if(_theme.tiles[terrain.id].prop.textures.size() > tile.owner)
  {
    glhckTexture* texture = glhckTextureNewFromFile(_theme.tiles[terrain.id].prop.textures[tile.owner].data(),
        glhckImportDefaultImageParameters(), glhckTextureDefaultLinearParameters());
    glhckMaterial* m = glhckObjectGetMaterial(o);
    if(m == nullptr)
    {
      m = glhckMaterialNew(texture);
      glhckObjectMaterial(o, m);
      glhckMaterialFree(m);
    }
    else
    {
      glhckMaterialTexture(m, texture);
    }
    glhckTextureFree(texture);
  }

  kmVec3 pos = {static_cast<kmScalar>(tile.x), static_cast<kmScalar>(tile.y), 0};
  kmVec3Add(&pos, &pos, &_theme.tiles[tile.type].offset);
  pos = hexToRect(pos);
  glhckObjectPositionf(o, pos.x, pos.y, pos.z);
  return o;


}

void wars::GlhckView::loadTheme(const std::string& themeFile)
{
  json::Value v = json::Value::parseFile(themeFile);

  json::Value xb = v.get("base").get("x");
  _theme.base.x.x = xb.at(0).doubleValue();
  _theme.base.x.y = xb.at(1).doubleValue();
  _theme.base.x.z = xb.at(2).doubleValue();

  json::Value yb = v.get("base").get("y");
  _theme.base.y.x = yb.at(0).doubleValue();
  _theme.base.y.y = yb.at(1).doubleValue();
  _theme.base.y.z = yb.at(2).doubleValue();

  json::Value zb = v.get("base").get("z");
  _theme.base.z.x = zb.at(0).doubleValue();
  _theme.base.z.y = zb.at(1).doubleValue();
  _theme.base.z.z = zb.at(2).doubleValue();

  _theme.playerColors.clear();
  json::Value pc = v.get("playerColors");
  for(int i = 0; i < pc.size(); ++i)
  {
    json::Value c = pc.at(i);
    glhckColorb color;
    color.r = c.at(0).longValue();
    color.g = c.at(1).longValue();
    color.b = c.at(2).longValue();
    color.a = c.at(3).longValue();
    _theme.playerColors.push_back(color);
  }

  _theme.tiles.clear();
  json::Value tiles = v.get("tiles");
  for(int i = 0; i < tiles.size(); ++i)
  {
    json::Value t = tiles.at(i);
    Theme::Tile tile = {"", {"", {}}, {0, 0, 0}};
    tile.model = t.get("model").stringValue();
    json::Value o = t.get("offset");
    if(o.type() != json::Value::Type::NONE)
    {
      tile.offset.x = o.at(0).doubleValue();
      tile.offset.y = o.at(1).doubleValue();
      tile.offset.z = o.at(2).doubleValue();
    }
    json::Value prop = t.get("prop");
    if(prop.type() != json::Value::Type::NONE)
    {
      tile.prop.model = prop.get("model").stringValue();
      json::Value textures = prop.get("textures");
      if(textures.type() != json::Value::Type::NONE)
      {
        for(int j = 0; j < textures.size(); ++j)
        {
          tile.prop.textures.push_back(textures.at(j).stringValue());
        }
      }
    }

    _theme.tiles.push_back(tile);
  }
}


