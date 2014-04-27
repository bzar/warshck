#include "hexlabel.h"
#include <sstream>
#include <string>

wars::HexLabel::Shared* wars::HexLabel::_shared = nullptr;
int wars::HexLabel::_sharedRefs = 0;

namespace
{
  glhckRect relativeRect(glhckRect const& rect, float const w, float const h);
}
wars::HexLabel::HexLabel(const Theme* theme) : _theme(theme), _obj(nullptr),
  _hex{{255, 255, 255, 255}, 0, false, false},
  _unit{{255, 255, 255, 255}, 0, 0, false, false, 0, 0}
{
  if(_sharedRefs == 0)
  {
    _shared = new Shared(_theme->hexLabel.image.file);
  }
  ++_sharedRefs;
}

wars::HexLabel::~HexLabel()
{
  if(_obj != nullptr)
    glhckObjectFree(_obj);

  --_sharedRefs;
  if(_sharedRefs <= 0)
  {
    delete _shared;
    _shared = nullptr;
  }
}

wars::HexLabel::HexLabel(const HexLabel& other) : _theme(other._theme),
  _obj(glhckObjectCopy(other._obj)), _hex(other._hex), _unit(other._unit)
{
  ++_sharedRefs;
}

wars::HexLabel& wars::HexLabel::operator=(const HexLabel& other)
{
  if(this != &other)
  {
    _theme = other._theme;
    _hex = other._hex;
    _unit = other._unit;

    if(_obj != nullptr)
      glhckObjectFree(_obj);

    _obj = glhckObjectCopy(other._obj);
  }

  return *this;
}

void wars::HexLabel::setHexInformation(const glhckColorb& ownerColor, int capturePoints, bool beingCaptured, bool hasUnit)
{
  _hex = {ownerColor, capturePoints, beingCaptured, hasUnit};
}

void wars::HexLabel::setUnitInformation(const glhckColorb& ownerColor, int health, int damage, bool deployed, bool capturing, int capacity, int carrying)
{
  _unit= {ownerColor, health, damage, deployed, capturing, capacity, carrying};
}

glhckObject* wars::HexLabel::getObject()
{
  return _obj;
}

void wars::HexLabel::refresh()
{
  float tWidth = 2 * _theme->hexLabel.image.unitBody.w;
  float tHeight = _theme->hexLabel.image.unitTitle.h + _theme->hexLabel.image.unitBody.h;
  float oWidth = 4;
  float oHeight = oWidth * tHeight / tWidth;

  if(_obj == nullptr)
  {
    _obj = glhckPlaneNew(oWidth, oHeight);
    glhckTexture *texture = glhckTextureNew();
    glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, tWidth, tHeight, 0, 0, GLHCK_RGBA, GLHCK_FLOAT, 0, NULL);
    glhckTextureParameter(texture, glhckTextureDefaultSpriteParameters());
    glhckObjectMaterial(_obj, glhckMaterialNew(texture));
  }

  glhckFramebuffer* fbo = glhckFramebufferNew(GLHCK_FRAMEBUFFER);
  glhckFramebufferAttachTexture(fbo, glhckMaterialGetTexture(glhckObjectGetMaterial(_obj)), GLHCK_COLOR_ATTACHMENT0);
  glhckFramebufferRecti(fbo, 0, 0, tWidth, tHeight);
  glhckFramebufferBegin(fbo);
  glhckRenderProjection2D(tWidth, tHeight, 0, 100);
  glhckRenderClearColorb(255, 0, 255, 0);
  glhckRenderClear(GLHCK_COLOR_BUFFER_BIT | GLHCK_DEPTH_BUFFER_BIT);
  float partWidth = tWidth / 2.0f;
  bool showHexLabel = _hex.capturePoints < 200;
  bool showUnitLabel = _hex.hasUnit;


  float titleHeight = _theme->hexLabel.image.unitTitle.h;
  float bodyHeight = _theme->hexLabel.image.unitBody.h;

  glhckTextClear(_shared->text);
  if(showUnitLabel)
  {
    // Unit title
    float x = showHexLabel ? partWidth / 2.0f : partWidth;
    glhckObject* unitTitle = glhckSpriteNew(_shared->texture, partWidth, titleHeight);
    glhckObjectPositionf(unitTitle, x, titleHeight / 2, 0);
    glhckRect unitTitleRect = relativeRect(_theme->hexLabel.image.unitTitle, _shared->textureWidth, _shared->textureHeight);
    glhckMaterialTextureTransform(glhckObjectGetMaterial(unitTitle), &unitTitleRect, 0);
    glhckMaterialDiffuse(glhckObjectGetMaterial(unitTitle), &_unit.ownerColor);
    glhckObjectDraw(unitTitle);
    glhckObjectFree(unitTitle);

    // Unit body
    glhckObject* unitBody = glhckSpriteNew(_shared->texture, partWidth, bodyHeight);
    glhckObjectPositionf(unitBody, x, titleHeight + bodyHeight / 2, 0);
    glhckRect unitBodyRect = relativeRect(_theme->hexLabel.image.unitBody, _shared->textureWidth, _shared->textureHeight);
    glhckMaterialTextureTransform(glhckObjectGetMaterial(unitBody), &unitBodyRect, 0);
    glhckObjectDraw(unitBody);
    glhckObjectFree(unitBody);

    float iconWidth = _theme->hexLabel.image.unitDeployIcon.h;
    float iconHeight = _theme->hexLabel.image.unitDeployIcon.h;

    // Unit deployed
    if(_unit.deployed)
    {
      float xx = x - partWidth / 2 + (3 * iconWidth / 4);
      float yy = 3 * iconHeight / 4;
      glhckObject* deployed = glhckSpriteNew(_shared->texture, iconWidth, iconHeight);
      glhckObjectPositionf(deployed, xx, yy, 0);
      glhckRect deployedRect = relativeRect(_theme->hexLabel.image.unitDeployIcon, _shared->textureWidth, _shared->textureHeight);
      glhckMaterialTextureTransform(glhckObjectGetMaterial(deployed), &deployedRect, 0);
      glhckObjectDraw(deployed);
      glhckObjectFree(deployed);
    }

    // Unit capturing
    if(_unit.capturing)
    {
      float xx = x - partWidth / 2 + (_unit.deployed ? 3 : 1) * (3 * iconWidth / 4);
      float yy = 3 * iconHeight / 4;
      glhckObject* capturing = glhckSpriteNew(_shared->texture, iconWidth, iconHeight);
      glhckObjectPositionf(capturing, xx, yy, 0);
      glhckRect capturingRect = relativeRect(_theme->hexLabel.image.unitCaptureIcon, _shared->textureWidth, _shared->textureHeight);
      glhckMaterialTextureTransform(glhckObjectGetMaterial(capturing), &capturingRect, 0);
      glhckObjectDraw(capturing);
      glhckObjectFree(capturing);
    }

    // Unit carry
    for(int i = 0; i < _unit.capacity; ++i)
    {
      bool filled = i < _unit.carrying;
      float xx = x + partWidth / 2 - (2 * i + 1) * (3 * iconWidth / 4);
      float yy = 3 * iconHeight / 4;
      glhckObject* carry = glhckSpriteNew(_shared->texture, iconWidth, iconHeight);
      glhckObjectPositionf(carry, xx, yy, 0);
      glhckRect carryRect = relativeRect(filled ? _theme->hexLabel.image.unitCarryUsedIcon : _theme->hexLabel.image.unitCarryFreeIcon, _shared->textureWidth, _shared->textureHeight);
      glhckMaterialTextureTransform(glhckObjectGetMaterial(carry), &carryRect, 0);
      glhckObjectDraw(carry);
      glhckObjectFree(carry);
    }

    // Health text
    std::ostringstream health;
    health << _unit.health;
    kmVec2 healthMin, healthMax;
    glhckTextGetMinMax(_shared->text, _shared->font, bodyHeight, health.str().data(), &healthMin, &healthMax);
    float healthPad = (partWidth - (healthMax.x - healthMin.x)) / 2;
    glhckTextStash(_shared->text, _shared->font, bodyHeight, x - partWidth/2 + healthPad, titleHeight + bodyHeight, health.str().data(), NULL);
  }

  if(showHexLabel)
  {
    float x = showUnitLabel ? 3 * partWidth / 2.0f : partWidth;
    // Hex title
    glhckObject* hexTitle = glhckSpriteNew(_shared->texture, partWidth, titleHeight);
    glhckObjectPositionf(hexTitle, x, titleHeight / 2, 0);
    glhckRect hexTitleRect = relativeRect(_theme->hexLabel.image.hexTitle, _shared->textureWidth, _shared->textureHeight);
    glhckMaterialTextureTransform(glhckObjectGetMaterial(hexTitle), &hexTitleRect, 0);
    glhckMaterialDiffuse(glhckObjectGetMaterial(hexTitle), &_hex.ownerColor);
    glhckObjectDraw(hexTitle);
    glhckObjectFree(hexTitle);

    // Hex body
    glhckObject* hexBody = glhckSpriteNew(_shared->texture, partWidth, bodyHeight);
    glhckObjectPositionf(hexBody, x, titleHeight + bodyHeight / 2, 0);
    glhckRect hexBodyRect = relativeRect(_hex.beingCaptured ? _theme->hexLabel.image.hexBodyCapture : _theme->hexLabel.image.hexBodyRegenerate, _shared->textureWidth, _shared->textureHeight);
    glhckMaterialTextureTransform(glhckObjectGetMaterial(hexBody), &hexBodyRect, 0);
    glhckObjectDraw(hexBody);
    glhckObjectFree(hexBody);

    // Capture points text
    std::ostringstream capturePoints;
    capturePoints << _hex.capturePoints;
    kmVec2 capturePointsMin, capturePointsMax;
    glhckTextGetMinMax(_shared->text, _shared->font, bodyHeight, capturePoints.str().data(), &capturePointsMin, &capturePointsMax);
    float capturePointsPad = (partWidth - (capturePointsMax.x - capturePointsMin.x)) / 2;
    glhckTextStash(_shared->text, _shared->font, bodyHeight, x - partWidth/2 + capturePointsPad, titleHeight + bodyHeight, capturePoints.str().data(), NULL);
 }

  glhckRender();
  glhckTextRender(_shared->text);

  glhckFramebufferEnd(fbo);
  glhckFramebufferFree(fbo);
}

bool wars::HexLabel::isVisible() const
{
  return _hex.capturePoints < 200 || _hex.hasUnit;
}


wars::HexLabel::Shared::Shared(const std::string& filename)
{
  texture = glhckTextureNewFromFile(filename.data(), glhckImportDefaultImageParameters(), glhckTextureDefaultLinearParameters());
  glhckTextureGetInformation(texture, NULL, &textureWidth, &textureHeight, NULL, NULL, NULL, NULL);
  text = glhckTextNew(512, 512);
  glhckTextColorb(text, 255, 255, 255, 255);
  font = glhckTextFontNewKakwafont(text, NULL);
}

wars::HexLabel::Shared::~Shared()
{
  glhckTextureFree(texture);
  glhckTextFree(text);
}
namespace
{
  glhckRect relativeRect(glhckRect const& rect, float const w, float const h)
  {
    return {
      rect.x / w,
      rect.y / h,
      rect.w / w,
      rect.h / h
    };
  }
}
