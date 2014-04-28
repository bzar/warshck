#ifndef THEME_H
#define THEME_H

#include <string>
#include <vector>
#include "glhck/glhck.h"

namespace wars
{

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

    struct
    {
      struct
      {
        std::string file;
        glhckRect unitTitle;
        glhckRect hexTitle;
        glhckRect unitBody;
        glhckRect unitBodyDamage;
        glhckRect hexBodyCapture;
        glhckRect hexBodyRegenerate;
        glhckRect unitCaptureIcon;
        glhckRect unitDeployIcon;
        glhckRect unitCarryFreeIcon;
        glhckRect unitCarryUsedIcon;
      } image;
    } hexLabel;
  };

  Theme loadTheme(std::string const& themeFile);
}
#endif // THEME_H
