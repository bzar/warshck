#ifndef WARS_VIEW_H
#define WARS_VIEW_H

#include "game.h"

namespace wars
{
  class View
  {
  public:
    virtual void setGame(Game* game) = 0;
    virtual bool handle() = 0;
  };
}
#endif // WARS_VIEW_H
