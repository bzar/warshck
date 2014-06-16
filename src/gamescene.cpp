#include "gamescene.h"


wars::GameScene::GameScene(wars::Game* game, Theme* theme) :
  _game(game), _theme(theme), _sky(nullptr), _units(), _tiles(), _eventSub()
{
  _eventSub = _game->events().on([this](wars::Game::Event const& e) {
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

        _tiles.at(next.id).labelUpdate = true;
        _tiles.at(prev.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::WAIT:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.wait.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        _tiles.at(curr.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::ATTACK:
      {
        wars::Game::Unit const& attacker = _game->getUnit(*e.attack.attackerId);
        wars::Game::Unit const& target = _game->getUnit(*e.attack.targetId);
        _tiles.at(attacker.tileId).labelUpdate = true;
        _tiles.at(target.tileId).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::COUNTERATTACK:
      {
        wars::Game::Unit const& attacker = _game->getUnit(*e.counterattack.attackerId);
        wars::Game::Unit const& target = _game->getUnit(*e.counterattack.targetId);
        _tiles.at(attacker.tileId).labelUpdate = true;
        _tiles.at(target.tileId).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::CAPTURE:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.capture.unitId);
        wars::Game::Tile const& tile = _game->getTile(*e.capture.tileId);
        _tiles.at(tile.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::CAPTURED:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.captured.unitId);
        wars::Game::Tile const& tile = _game->getTile(*e.captured.tileId);
        auto tileIter = _tiles.find(tile.id);
        if(tileIter != _tiles.end()  && tileIter->second.prop != nullptr)
        {
          updatePropTexture(tileIter->second.prop, tile.type, unit.owner);
        }
        _tiles.at(tile.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::DEPLOY:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.deploy.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        _tiles.at(curr.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::UNDEPLOY:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.undeploy.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        _tiles.at(curr.id).labelUpdate = true;
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
        _tiles.at(curr.id).labelUpdate = true;
        _tiles.at(carrier.tileId).labelUpdate = true;
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
        _tiles.at(carrier.tileId).labelUpdate = true;
        _tiles.at(next.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::DESTROY:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.destroy.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        glhckObject* o = _units.at(unit.id).obj;
        _units.erase(unit.id);
        glhckObjectFree(o);
        _tiles.at(curr.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::REPAIR:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.repair.unitId);
        wars::Game::Tile const& curr = _game->getTile(unit.tileId);
        _tiles.at(curr.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::BUILD:
      {
        wars::Game::Unit const& unit = _game->getUnit(*e.build.unitId);
        wars::Game::Tile const& tile = _game->getTile(*e.build.tileId);
        glhckObject* unitObject = createUnitObject(unit);
        if(unitObject != nullptr)
          _units[unit.id] = {unit.id, unitObject};
        _tiles.at(tile.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::REGENERATE_CAPTURE_POINTS:
      {
        wars::Game::Tile const& tile = _game->getTile(*e.regenerateCapturePoints.tileId);
        _tiles.at(tile.id).labelUpdate = true;
        break;
      }
      case wars::Game::EventType::PRODUCE_FUNDS:
      {
        wars::Game::Tile const& tile = _game->getTile(*e.produceFunds.tileId);
        _tiles.at(tile.id).labelUpdate = true;
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

void wars::GameScene::render()
{
  if(_sky != nullptr)
    glhckObjectDraw(_sky);

  glhckRenderClear(GLHCK_DEPTH_BUFFER_BIT | GLHCK_COLOR_BUFFER_BIT);

  bool tilesHighlighted = false;
  for(auto& item : _tiles)
  {
    Game::Tile const& tile = _game->getTile(item.first);

    bool selected = false; /*tile.x == _inputState.hexCursor.x
        && tile.y == _inputState.hexCursor.y;*/
    glhckObjectDrawAABB(item.second.hex, selected);
    glhckObjectDraw(item.second.hex);

    tilesHighlighted |= item.second.effects.highlight;
  }
  glhckRender();

  if(tilesHighlighted)
  {
    glhckRenderBlendFunc(GLHCK_ONE, GLHCK_ONE);
    for(auto& item : _tiles)
    {
      if(item.second.effects.highlight)
      {
        glhckObjectDraw(item.second.hex);
      }
    }

    glhckRender();
    glhckRenderBlendFunc(GLHCK_ZERO, GLHCK_ZERO);
  }

  for(auto& item : _tiles)
  {
    if(item.second.prop != nullptr)
      glhckObjectDraw(item.second.prop);
  }
  glhckRender();

  if(tilesHighlighted)
  {
    glhckRenderBlendFunc(GLHCK_ONE, GLHCK_ONE);
    for(auto& item : _tiles)
    {
      if(item.second.effects.highlight)
      {
        if(item.second.prop != nullptr)
          glhckObjectDraw(item.second.prop);
      }
    }

    glhckRender();
    glhckRenderBlendFunc(GLHCK_ZERO, GLHCK_ZERO);
  }

  bool unitsHighlighted = false;
  for(auto& item : _units)
  {
    Game::Unit const& unit = _game->getUnit(item.first);
    unitsHighlighted |= item.second.effects.highlight;

    glhckColorb diffuse = _theme->playerColors[unit.owner];
    if(unit.moved)
    {
      diffuse.r *= 0.5;
      diffuse.g *= 0.5;
      diffuse.b *= 0.5;
    }
    glhckMaterialDiffuse(glhckObjectGetMaterial(item.second.obj), &diffuse);
    glhckObjectDraw(item.second.obj);
  }

  glhckRender();

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

  glhckRenderStatePush();
  glhckRenderClear(GLHCK_DEPTH_BUFFER_BIT);
  glhckRenderBlendFunc(GLHCK_SRC_ALPHA, GLHCK_ONE_MINUS_SRC_ALPHA);

  for(auto& item : _tiles)
  {
    if(item.second.labelUpdate)
    {
      updateHexLabel(item.first);
      item.second.labelUpdate = false;
    }

    if(item.second.label.isVisible())
    {
      const kmVec3* pos = glhckObjectGetPosition(item.second.hex);
      glhckObjectPosition(item.second.label.getObject(), pos);
      glhckObjectRotationf(item.second.label.getObject(), 45, 0, 0);
      glhckObjectMovef(item.second.label.getObject(), 0, -1.5, 1);
      glhckObjectDraw(item.second.label.getObject());
    }
  }

  glhckRender();
  glhckRenderStatePop();
}

kmVec3 wars::GameScene::hexToRect(const kmVec3& v)
{
  kmVec3 result, r;
  kmVec3Scale(&result, &_theme->base.x, v.x);
  kmVec3Scale(&r, &_theme->base.y, v.y);
  kmVec3Add(&result, &result, &r);
  kmVec3Scale(&r, &_theme->base.z, v.z);
  kmVec3Add(&result, &result, &r);
  return result;
}

kmVec3 wars::GameScene::rectToHex(const kmVec3& v)
{
  kmVec3 result;
  kmMat4 mat = {
    _theme->base.x.x, _theme->base.x.y, _theme->base.x.z, 0,
    _theme->base.y.x, _theme->base.y.y, _theme->base.y.z, 0,
    _theme->base.z.x, _theme->base.z.y, _theme->base.z.z, 0,
    0, 0, 0, 1
  };

  kmMat4Inverse(&mat, &mat);

  kmVec3Transform(&result, &v, &mat);
  return result;
}

void wars::GameScene::setHighlightedTiles(std::vector<wars::Game::Coordinates> const& coords)
{
  for(auto& item : _tiles)
  {
    Game::Tile const& t = _game->getTile(item.first);
    Game::Coordinates c = {t.x, t.y};
    item.second.effects.highlight = std::find(coords.begin(), coords.end(), c) != coords.end();
  }
}

void wars::GameScene::clearHighlightedTiles()
{
  setHighlightedTiles(std::vector<Game::Coordinates>());
}

void wars::GameScene::setAttackOptions(std::unordered_map<std::string, int> const& options)
{
  for(auto& item : _units)
  {
    auto iter = options.find(item.first);
    if(iter != options.end())
    {
      item.second.effects.highlight = true;
      Tile& tile = _tiles.at(_game->getUnit(item.first).tileId);
      tile.label.setUnitDamage(iter->second);
      tile.label.refresh();
    }
  }
}

void wars::GameScene::clearAttackOptions(std::unordered_map<std::string, int> const& options)
{
  for(auto& item : _units)
  {
    if(options.find(item.first) != options.end())
    {
      item.second.effects.highlight = false;
      Tile& tile = _tiles.at(_game->getUnit(item.first).tileId);
      tile.label.setUnitDamage(0);
      tile.label.refresh();
    }
  }
}

void wars::GameScene::initializeFromGame()
{
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

    Tile tile = {
      item.first,
      createTileHex(item.second),
      createTileProp(item.second),
      false,
      HexLabel(_theme)
    };

    tile.label.setHexInformation(_theme->playerColors.at(item.second.owner), item.second.capturePoints, item.second.beingCaptured, !item.second.unitId.empty());
    if(!item.second.unitId.empty())
    {
      Game::Unit const& unit = units.at(item.second.unitId);
      UnitType const& unitType = _game->getRules().unitTypes.at(unit.type);
      tile.label.setUnitInformation(_theme->playerColors.at(unit.owner), unit.health, 0, unit.deployed, unit.capturing, unitType.carryNum, unit.carriedUnits.size());
    }
    tile.label.refresh();
    _tiles.insert(make_pair(item.first, tile));
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

}

void wars::GameScene::updatePropTexture(glhckObject* o, int terrainId, int owner)
{
  if(_theme->tiles[terrainId].prop.textures.size() > owner)
  {
    glhckTexture* texture = glhckTextureNewFromFile(_theme->tiles[terrainId].prop.textures[owner].data(),
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
}

void wars::GameScene::updateHexLabel(const std::string& id)
{
  Tile& t = _tiles.at(id);
  Game::Tile const& tile = _game->getTile(t.id);
  t.label.setHexInformation(_theme->playerColors.at(tile.owner), tile.capturePoints, tile.beingCaptured, !tile.unitId.empty());

  if(!tile.unitId.empty())
  {
    Game::Unit const& unit = _game->getUnit(tile.unitId);
    UnitType const& unitType = _game->getRules().unitTypes.at(unit.type);
    t.label.setUnitInformation(_theme->playerColors.at(unit.owner), unit.health, 0, unit.deployed, unit.capturing, unitType.carryNum, unit.carriedUnits.size());
  }
  t.label.refresh();
}

glhckObject*wars::GameScene::createUnitObject(const wars::Game::Unit& unit)
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
  glhckMaterialDiffuse(m, &_theme->playerColors[unit.owner]);
  glhckObjectMaterial(o, m);
  glhckMaterialFree(m);
  glhckObjectDrawOBB(o, 1);
  return o;
}

glhckObject*wars::GameScene::createTileHex(const wars::Game::Tile& tile)
{
  TerrainType const& terrain = _game->getRules().terrainTypes.at(tile.type);
  glhckObject* o = glhckModelNew(_theme->tiles[tile.type].model.data(), 1.0, glhckImportDefaultModelParameters());
  kmVec3 pos = {static_cast<kmScalar>(tile.x), static_cast<kmScalar>(tile.y), 0};
  kmVec3Add(&pos, &pos, &_theme->tiles[tile.type].offset);
  pos = hexToRect(pos);
  glhckObjectPositionf(o, pos.x, pos.y, pos.z);
  return o;
}

glhckObject*wars::GameScene::createTileProp(const wars::Game::Tile& tile)
{
  TerrainType const& terrain = _game->getRules().terrainTypes.at(tile.type);
  if(_theme->tiles[terrain.id].prop.model.empty())
    return nullptr;

  glhckObject* o = glhckModelNew(_theme->tiles[terrain.id].prop.model.data(), 1.0, glhckImportDefaultModelParameters());

  updatePropTexture(o, terrain.id, tile.owner);

  kmVec3 pos = {static_cast<kmScalar>(tile.x), static_cast<kmScalar>(tile.y), 0};
  kmVec3Add(&pos, &pos, &_theme->tiles[tile.type].offset);
  pos = hexToRect(pos);
  glhckObjectPositionf(o, pos.x, pos.y, pos.z);
  return o;
}


