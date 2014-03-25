#include "glhckview.h"
#include <iostream>
#include <stdexcept>

namespace
{
  void errorCallback(int code, char const* message)
  {
    std::cerr << "GLFW ERROR: " << message << std::endl;
  }
}


kmVec3 const wars::GlhckView::_xBase = {4.2, -2.2, 0};
kmVec3 const wars::GlhckView::_yBase = {0, -4.4, 0};
kmVec3 const wars::GlhckView::_zBase = {0, 0, 1};

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

wars::GlhckView::GlhckView(Gamenode* gn) : _gn(gn), _window(nullptr), _shouldQuit(false), _units(), _tiles()
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
}

wars::GlhckView::~GlhckView()
{
  glfwhckEventQueueFree(_glfwEvents);
}

void wars::GlhckView::setGame(Game* game)
{
  _game = game;

  _game->events().on<void>([this](wars::Game::Event const& e) {
    switch(e.type)
    {
      case wars::Game::EventType::GAMEDATA:
      {
        initializeFromGame();
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

  for(auto& item : _units)
  {
    glhckObjectDraw(item.second.obj);
  }

  for(auto& item : _tiles)
  {
    Game::Tile const& tile = _game->getTile(item.first);
    bool selected = tile.x == _inputState.hexCursor.x
        && tile.y == _inputState.hexCursor.y;
    glhckObjectDrawAABB(item.second.hex, selected);
    glhckObjectDraw(item.second.hex);
    if(item.second.prop != nullptr)
      glhckObjectDraw(item.second.prop);
  }

  glhckRenderClear(GLHCK_DEPTH_BUFFER_BIT | GLHCK_COLOR_BUFFER_BIT);
  glhckRender();
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
  kmVec3Scale(&result, &_xBase, v.x);
  kmVec3Scale(&r, &_yBase, v.y);
  kmVec3Add(&result, &result, &r);
  kmVec3Scale(&r, &_zBase, v.z);
  kmVec3Add(&result, &result, &r);
  return result;
}

kmVec3 wars::GlhckView::rectToHex(const kmVec3& v)
{

  kmVec3 result;
  kmMat4 mat = {
    _xBase.x, _xBase.y, _xBase.z, 0,
    _yBase.x, _yBase.y, _yBase.z, 0,
    _zBase.x, _zBase.y, _zBase.z, 0,
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
}

void wars::GlhckView::initializeFromGame()
{
  clear();

  std::unordered_map<std::string, Game::Tile> const& tiles = _game->getTiles();
  std::unordered_map<std::string, Game::Unit> const& units = _game->getUnits();
  Rules const& rules = _game->getRules();

  for(auto item : tiles)
  {
    _tiles[item.first] = {
      item.first,
      createTileHex(item.second),
      createTileProp(item.second)
    };
  }

  for(auto item : units)
  {
    if(!item.second.tileId.empty())
    {
      glhckObject* unitObject = createUnitObject(item.second);
      _units[item.first] = {item.first, unitObject};
    }
  }

}

void wars::GlhckView::handleInput()
{
  while(!glfwhckEventQueueEmpty(_glfwEvents))
  {
    const glfwhckEvent* e = glfwhckEventQueuePop(_glfwEvents);
    switch(e->type)
    {
      case GLFWHCK_EVENT_MOUSE_POSITION:
      {
        int w, h;
        glfwGetWindowSize(_window, &w, &h);
        glhckCameraCastRayFromPointf(_camera, &_inputState.mouseRay,
                                     e->mousePosition.x/w,
                                     1.0f - e->mousePosition.y/h);
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

  if(_inputState.mouseRay.dir.z)
  {
    kmVec3 pointer;
    kmPlane plane = {0, 0, 1, 0};
    kmRay3IntersectPlane(&pointer, &_inputState.mouseRay, &plane);

    pointer = rectToHex(pointer);
    _inputState.hexCursor.x = static_cast<int>(round(pointer.x));
    _inputState.hexCursor.y = static_cast<int>(round(pointer.y));
  }
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
  glhckColorb colors[] = {
    {127, 127, 127, 255},
    {214,  61,  56, 255},
    { 56,  67, 214, 255},
    {217, 213,  43, 255},
    { 99, 173, 208, 255},
    {230,   0, 211, 255},
    {230, 108,   0, 255},
    {177, 108,  53, 255},
    {173, 230,   0, 255}
  };
  glhckMaterialDiffuse(m, &colors[unit.owner]);
  glhckObjectMaterial(o, m);
  glhckMaterialFree(m);
  glhckObjectDrawOBB(o, 1);
  return o;
}

glhckObject* wars::GlhckView::createTileHex(const wars::Game::Tile& tile)
{
  TerrainType const& terrain = _game->getRules().terrainTypes.at(tile.type);
  std::string const files[] = {
    "models/grass-hextile.glhckm", // road
    "models/grass-hextile.glhckm", // plains
    "models/grass-hextile.glhckm", // forest
    "models/grass-hextile.glhckm", // mountains
    "models/water-hextile.glhckm", // water
    "models/concrete-hextile.glhckm", // city
    "models/concrete-hextile.glhckm", // base
    "models/concrete-hextile.glhckm", // fort
    "models/concrete-hextile.glhckm", // airport
    "models/concrete-hextile.glhckm", // port
    "models/beach-hextile.glhckm", // beach
    "models/water-hextile.glhckm", // bridge
    "models/concrete-hextile.glhckm" // hq
  };
  glhckObject* o = glhckModelNew(files[terrain.id].data(), 1.0, glhckImportDefaultModelParameters());
  kmVec3 pos = hexToRect({static_cast<kmScalar>(tile.x), static_cast<kmScalar>(tile.y), 0});
  glhckObjectPositionf(o, pos.x, pos.y, pos.z);
  return o;
}

glhckObject*wars::GlhckView::createTileProp(const wars::Game::Tile& tile)
{
  TerrainType const& terrain = _game->getRules().terrainTypes.at(tile.type);
  std::string const files[] = {
    "models/road.glhckm", // road
    "", // plains
    "models/forest.glhckm", // forest
    "", // mountains
    "", // water
    "models/city-hextile.glhckm", // city
    "", // base
    "", // fort
    "", // airport
    "", // port
    "", // beach
    "", // bridge
    "" // hq
  };
  if(files[terrain.id].empty())
    return nullptr;

  glhckObject* o = glhckModelNew(files[terrain.id].data(), 1.0, glhckImportDefaultModelParameters());
  kmVec3 pos = hexToRect({static_cast<kmScalar>(tile.x), static_cast<kmScalar>(tile.y), 0});
  glhckObjectPositionf(o, pos.x, pos.y, pos.z);
  return o;


}
