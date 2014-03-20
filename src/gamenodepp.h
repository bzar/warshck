#ifndef GAMENODEPP_H
#define GAMENODEPP_H

#include "gamenode.h"
#include "jsonpp.h"
#include <string>
#include <functional>
#include <map>
#include <vector>

class Gamenode
{
public:
  typedef std::function<void(JSONValue const&)> MethodCallback;
  typedef std::function<JSONValue(JSONValue const&)> Method;
  typedef std::function<void()> ConnectCallback;
  typedef std::function<void()> DisconnectCallback;
  typedef std::function<void()> ErrorCallback;

  Gamenode();
  ~Gamenode();

  void onConnect(ConnectCallback callback);
  void onDisconnect(DisconnectCallback callback);
  void onError(ErrorCallback callback);
  void onMethod(std::string const& methodName, Method method);

  bool connect(std::string const& address,
               int port,
               std::string const& path,
               std::string const& host,
               std::string const& origin);
  void disconnect();
  bool handle();

  long int call(std::string const& methodName, JSONValue const& params, MethodCallback callback);

private:
  static void gamenodeCallback(gamenode* gn, gamenodeEvent const* event);
  void handleResponse(long msgId, JSON_Value* value);
  void handleMethodCall(long msgId, std::string const& methodName, JSON_Value* params);

  gamenode* _gn;
  std::map<long int, MethodCallback> _callbacks;
  std::map<std::string, Method> _methods;
  ConnectCallback _connectCallback;
  DisconnectCallback _disconnectCallback;
  ErrorCallback _errorCallback;
};

Gamenode::Gamenode() : _gn(gamenodeNew(gamenodeCallback)), _callbacks()
{
  gamenodeSetUserData(_gn, this);
}

Gamenode::~Gamenode()
{
  gamenodeFree(_gn);
}

void Gamenode::onConnect(ConnectCallback callback)
{
  _connectCallback = callback;
}

void Gamenode::onDisconnect(DisconnectCallback callback)
{
  _disconnectCallback = callback;
}

void Gamenode::onError(ErrorCallback callback)
{
  _errorCallback = callback;
}

void Gamenode::onMethod(std::string const& methodName, Method method)
{
  _methods[methodName] = method;
  std::vector<const char*> methodList;
  for(auto& pair : _methods)
  {
    methodList.push_back(pair.first.data());
  }
  gamenodeSetMethodNames(_gn, methodList.data(), methodList.size());
}

bool Gamenode::connect(std::string const& address,
                       int port,
                       std::string const& path,
                       std::string const& host,
                       std::string const& origin)
{
  return gamenodeConnect(_gn, address.data(), port, path.data(), host.data(), origin.data()) == 0;
}

void Gamenode::disconnect()
{
  gamenodeDisconnect(_gn);
}

bool Gamenode::handle()
{
  return gamenodeHandle(_gn) == 0;
}

long int Gamenode::call(std::string const& methodName, JSONValue const& params, Gamenode::MethodCallback callback)
{
  JSONValue paramsClone = params;
  long msgId = gamenodeMethodCall(_gn, methodName.data(), paramsClone.extract());
  _callbacks[msgId] = callback;
  return msgId;
}

void Gamenode::handleResponse(long msgId, JSON_Value* value)
{
  auto iter = _callbacks.find(msgId);
  if(iter != _callbacks.end())
  {
    auto& callback = iter->second;
    JSONValue v(JSON_Value_Clone(value));
    callback(v);
    _callbacks.erase(iter);
  }
}

void Gamenode::handleMethodCall(long msgId, std::string const& methodName, JSON_Value* params)
{
  auto iter = _methods.find(methodName);
  if(iter != _methods.end())
  {
    auto& callback = iter->second;
    JSONValue p(JSON_Value_Clone(params));
    JSONValue ret = callback(p);
    gamenodeResponse(_gn, msgId, ret.extract());
  }
}

void Gamenode::gamenodeCallback(gamenode* gn, gamenodeEvent const* event)
{
  Gamenode* gnpp = static_cast<Gamenode*>(gamenodeUserData(gn));

  switch(event->type)
  {
    case GAMENODE_CONNECTED:
    {
      gnpp->_connectCallback();
      break;
    }
    case GAMENODE_DISCONNECTED: break;
    {
      gnpp->_disconnectCallback();
      break;
    }
    case GAMENODE_ERROR: break;
    {
      gnpp->_errorCallback();
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

#endif // GAMENODEPP_H
