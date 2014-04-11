#ifndef INPUT_H
#define INPUT_H
#include "stream.h"
#include "promise.h"
#include <string>

namespace wars
{
  class Input
  {
  public:
    struct Build
    {
      std::string gameId;
      int x;
      int y;
      int type;
      Promise<bool> result;
    };

    struct
    {
      Stream<Build> build;
    } events;

    Promise<bool> build(std::string const& gameId, int x, int y, int type)
    {
      Promise<bool> promise;
      events.build.push({gameId, x, y, type, promise});
      return promise;
    }

  };
}
#endif // INPUT_H
