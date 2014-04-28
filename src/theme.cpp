#include "theme.h"
#include "jsonpp.h"

namespace {
  glhckRect parseRect(json::Value const& v);
}
wars::Theme wars::loadTheme(std::string const& themeFile)
{
  Theme theme;
  json::Value v = json::Value::parseFile(themeFile);

  json::Value xb = v.get("base").get("x");
  theme.base.x.x = xb.at(0).doubleValue();
  theme.base.x.y = xb.at(1).doubleValue();
  theme.base.x.z = xb.at(2).doubleValue();

  json::Value yb = v.get("base").get("y");
  theme.base.y.x = yb.at(0).doubleValue();
  theme.base.y.y = yb.at(1).doubleValue();
  theme.base.y.z = yb.at(2).doubleValue();

  json::Value zb = v.get("base").get("z");
  theme.base.z.x = zb.at(0).doubleValue();
  theme.base.z.y = zb.at(1).doubleValue();
  theme.base.z.z = zb.at(2).doubleValue();

  theme.playerColors.clear();
  json::Value pc = v.get("playerColors");
  for(int i = 0; i < pc.size(); ++i)
  {
    json::Value c = pc.at(i);
    glhckColorb color;
    color.r = c.at(0).longValue();
    color.g = c.at(1).longValue();
    color.b = c.at(2).longValue();
    color.a = c.at(3).longValue();
    theme.playerColors.push_back(color);
  }

  theme.tiles.clear();
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

    theme.tiles.push_back(tile);
  }

  json::Value hli = v.get("hexLabel").get("image");
  theme.hexLabel.image.file = hli.get("file").stringValue();
  theme.hexLabel.image.unitTitle = parseRect(hli.get("unitTitle"));
  theme.hexLabel.image.hexTitle = parseRect(hli.get("hexTitle"));
  theme.hexLabel.image.unitBody = parseRect(hli.get("unitBody"));
  theme.hexLabel.image.unitBodyDamage = parseRect(hli.get("unitBodyDamage"));
  theme.hexLabel.image.hexBodyCapture = parseRect(hli.get("hexBodyCapture"));
  theme.hexLabel.image.hexBodyRegenerate = parseRect(hli.get("hexBodyRegenerate"));
  theme.hexLabel.image.unitCaptureIcon = parseRect(hli.get("unitCaptureIcon"));
  theme.hexLabel.image.unitDeployIcon = parseRect(hli.get("unitDeployIcon"));
  theme.hexLabel.image.unitCarryFreeIcon = parseRect(hli.get("unitCarryFreeIcon"));
  theme.hexLabel.image.unitCarryUsedIcon = parseRect(hli.get("unitCarryUsedIcon"));

  return theme;
}

namespace
{
  glhckRect parseRect(json::Value const& v)
  {
    return {
      static_cast<float>(v.get("x").doubleValue()),
          static_cast<float>(v.get("y").doubleValue()),
          static_cast<float>(v.get("w").doubleValue()),
          static_cast<float>(v.get("h").doubleValue())
    };
  }
}
