#ifndef HEXLABEL_H
#define HEXLABEL_H

#include "glhck/glhck.h"
#include "theme.h"

namespace wars
{
  class HexLabel
  {
  public:
    HexLabel(Theme const* theme);
    ~HexLabel();

    HexLabel(HexLabel const& other);
    HexLabel& operator=(HexLabel const& other);

    void setHexInformation(glhckColorb const& ownerColor, int capturePoints, bool beingCaptured, bool hasUnit);
    void setUnitInformation(glhckColorb const& ownerColor, int health, int damage, bool deployed, bool capturing, int capacity, int carrying);

    glhckObject* getObject();
    void refresh();
    bool isVisible() const;

  private:
    Theme const* _theme;
    glhckObject* _obj;

    struct
    {
      glhckColorb ownerColor;
      int capturePoints;
      bool beingCaptured;
      bool hasUnit;
    } _hex;

    struct
    {
      glhckColorb ownerColor;
      int health;
      int damage;
      bool deployed;
      bool capturing;
      int capacity;
      int carrying;
    } _unit;

    struct Shared
    {
      Shared(std::string const& filename);
      ~Shared();

      glhckTexture* texture;
      int textureWidth;
      int textureHeight;
      glhckText* text;
      unsigned int font;
    };

    static Shared* _shared;
    static int _sharedRefs;
  };
}
#endif // HEXLABEL_H
