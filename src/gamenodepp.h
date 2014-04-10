#ifndef GAMENODEPP_H
#define GAMENODEPP_H

#include "gamenode.h"
#include "jsonpp.h"
#include <string>
#include <functional>
#include <map>
#include <vector>
#include "stream.h"
#include "promise.h"

class Gamenode
{
public:
  typedef std::function<json::Value(json::Value const&)> Method;
  typedef std::function<void(json::Value const&)> VoidMethod;
  Gamenode() : _gn(gamenodeNew(gamenodeCallback)), _callbacks()
  {
    gamenodeSetUserData(_gn, this);
  }

  ~Gamenode()
  {
    gamenodeFree(_gn);
  }


  void onMethod(std::string const& methodName, Method method)
  {
    _methods[methodName] = method;
    std::vector<const char*> methodList;
    for(auto& pair : _methods)
    {
      methodList.push_back(pair.first.data());
    }
    gamenodeSetMethodNames(_gn, methodList.data(), methodList.size());
  }

  void onVoidMethod(std::string const& methodName, VoidMethod method)
  {
    _methods[methodName] = [method](json::Value const& v) mutable {
      method(v);
      return json::Value::null();
    };

    std::vector<const char*> methodList;
    for(auto& pair : _methods)
    {
      methodList.push_back(pair.first.data());
    }
    gamenodeSetMethodNames(_gn, methodList.data(), methodList.size());
  }

  Stream<void> connected()
  {
    return _connected;
  }

  Stream<void> disconnected()
  {
    return _disconnected;
  }

  Stream<void> error()
  {
    return _error;
  }
  Stream<json::Value> method(std::string const& methodName);

  bool connect(std::string const& address,
               int port,
               std::string const& path,
               std::string const& host,
               std::string const& origin)
  {
    return gamenodeConnect(_gn, address.data(), port, path.data(), host.data(), origin.data()) == 0;
  }

  void disconnect()
  {
    gamenodeDisconnect(_gn);
  }

  bool handle()
  {
    return gamenodeHandle(_gn) == 0;
  }


  Promise<json::Value> call(std::string const& methodName, json::Value const& params)
  {
    json::Value paramsClone = params;
    long msgId = gamenodeMethodCall(_gn, methodName.data(), paramsClone.extract());
    Promise<json::Value> promise;
    _callbacks[msgId] = promise;
    return promise;
  }


private:
  static void gamenodeCallback(gamenode* gn, gamenodeEvent const* event)
  {
    Gamenode* gnpp = static_cast<Gamenode*>(gamenodeUserData(gn));

    switch(event->type)
    {
      case GAMENODE_CONNECTED:
      {
        gnpp->_connected.push();
        break;
      }
      case GAMENODE_DISCONNECTED:
      {
        gnpp->_disconnected.push();
        break;
      }
      case GAMENODE_ERROR:
      {
        gnpp->_error.push();
        break;
      }
      case GAMENODE_RESPONSE:
      {
        gnpp->handleResponse(event->response.id, event->response.value);
        break;
      }
      case GAMENODE_METHOD_CALL:
      {
        gnpp->handleMethodCall(event->methodCall.id, event->methodCall.methodName, event->methodCall.params);
        break;
      }
    }
  }

  void handleResponse(long msgId, chckJson* value)
  {
    auto iter = _callbacks.find(msgId);
    if(iter != _callbacks.end())
    {
      Promise<json::Value> promise = iter->second;
      json::Value v(chckJsonCopy(value));
      promise.fulfill(v);
      _callbacks.erase(iter);
    }
  }
  void handleMethodCall(long msgId, std::string const& methodName, chckJson* params)
  {
    auto iter = _methods.find(methodName);
    if(iter != _methods.end())
    {
      auto& callback = iter->second;
      json::Value p(chckJsonCopy(params));
      json::Value ret = callback(p);
      if(ret.type() != json::Value::Type::NONE)
        gamenodeResponse(_gn, msgId, ret.extract());
    }
  }


  gamenode* _gn;
  std::map<std::string, Method> _methods;
  std::map<long, Promise<json::Value>> _callbacks;
  Stream<void> _connected;
  Stream<void> _disconnected;
  Stream<void> _error;
};
#endif // GAMENODEPP_H
