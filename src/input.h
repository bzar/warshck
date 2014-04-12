#ifndef INPUT_H
#define INPUT_H
#include "stream.h"
#include "promise.h"
#include <string>
#include <vector>

namespace wars
{
  class Input
  {
  public:
    struct Position
    {
      int x;
      int y;
    };
    typedef std::vector<Position> Path;

    struct Build
    {
      std::string gameId;
      Position position;
      int type;
      Promise<bool> result;
    };

    struct MoveWait
    {
      std::string gameId;
      std::string unitId;
      Position destination;
      Path path;
      Promise<bool> result;
    };

    struct MoveAttack
    {
      std::string gameId;
      std::string unitId;
      std::string targetId;
      Position destination;
      Path path;
      Promise<bool> result;
    };

    typedef MoveWait MoveCapture;
    typedef MoveWait MoveDeploy;

    struct Undeploy
    {
      std::string gameId;
      std::string unitId;
      Promise<bool> result;
    };

    struct MoveLoad
    {
      std::string gameId;
      std::string unitId;
      std::string carrierId;
      Path path;
      Promise<bool> result;
    };

    struct MoveUnload
    {
      std::string gameId;
      std::string unitId;
      Position destination;
      Path path;
      std::string carriedId;
      Position unloadDestination;
      Promise<bool> result;
    };

    struct EndTurn
    {
      std::string gameId;
      Promise<bool> result;
    };

    struct Surrender
    {
      std::string gameId;
      Promise<bool> result;
    };

    struct
    {
      Stream<Build> build;
      Stream<MoveWait> moveWait;
      Stream<MoveAttack> moveAttack;
      Stream<MoveDeploy> moveDeploy;
      Stream<MoveCapture> moveCapture;
      Stream<MoveLoad> moveLoad;
      Stream<MoveUnload> moveUnload;
      Stream<Undeploy> undeploy;
      Stream<EndTurn> endTurn;
      Stream<Surrender> surrender;
    } events;

    Promise<bool> build(std::string const& gameId, Position position, int type)
    {
      Promise<bool> promise;
      events.build.push({gameId, position, type, promise});
      return promise;
    }
    Promise<bool> moveWait(std::string const& gameId, std::string const& unitId, Position destination, Path const& path)
    {
      Promise<bool> promise;
      events.moveWait.push({gameId, unitId, destination, path, promise});
      return promise;
    }
    Promise<bool> moveAttack(std::string const& gameId, std::string const& unitId, std::string const& targetId, Position destination, Path const& path)
    {
      Promise<bool> promise;
      events.moveAttack.push({gameId, unitId, targetId, destination, path, promise});
      return promise;
    }
    Promise<bool> moveDeploy(std::string const& gameId, std::string const& unitId, Position destination, Path const& path)
    {
      Promise<bool> promise;
      events.moveDeploy.push({gameId, unitId, destination, path, promise});
      return promise;
    }
    Promise<bool> undeploy(std::string const& gameId, std::string const& unitId)
    {
      Promise<bool> promise;
      events.undeploy.push({gameId, unitId, promise});
      return promise;
    }
    Promise<bool> moveCapture(std::string const& gameId, std::string const& unitId, Position destination, Path const& path)
    {
      Promise<bool> promise;
      events.moveCapture.push({gameId, unitId, destination, path, promise});
      return promise;
    }
    Promise<bool> moveLoad(std::string const& gameId, std::string const& unitId, std::string const& carrierId, Path const& path)
    {
      Promise<bool> promise;
      events.moveLoad.push({gameId, unitId, carrierId, path, promise});
      return promise;
    }
    Promise<bool> moveUnload(std::string const& gameId, std::string const& unitId, Position destination, Path const& path, std::string const& carriedId, Position carriedDestination)
    {
      Promise<bool> promise;
      events.moveUnload.push({gameId, unitId, destination, path, carriedId, carriedDestination, promise});
      return promise;
    }
    Promise<bool> endTurn(std::string const& gameId)
    {
      Promise<bool> promise;
      events.endTurn.push({gameId, promise});
      return promise;
    }
    Promise<bool> surrender(std::string const& gameId)
    {
      Promise<bool> promise;
      events.surrender.push({gameId, promise});
      return promise;
    }
  };
}
#endif // INPUT_H
