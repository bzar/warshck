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
  //typedef std::function<void(JSONValue const&)> MethodCallback;
  typedef std::function<JSONValue(JSONValue const&)> Method;
/*  typedef std::function<void()> ConnectCallback;
  typedef std::function<void()> DisconnectCallback;
  typedef std::function<void()> ErrorCallback;
*/
  Gamenode();
  ~Gamenode();

/*  void onConnect(ConnectCallback callback);
  void onDisconnect(DisconnectCallback callback);
  void onError(ErrorCallback callback);*/
  void onMethod(std::string const& methodName, Method method);

  Stream<void> connected();
  Stream<void> disconnected();
  Stream<void> error();
  Stream<JSONValue> method(std::string const& methodName);

  bool connect(std::string const& address,
               int port,
               std::string const& path,
               std::string const& host,
               std::string const& origin);
  void disconnect();
  bool handle();

  Promise<JSONValue> call(std::string const& methodName, JSONValue const& params);
  ///long int call(std::string const& methodName, JSONValue const& params, MethodCallback callback);

private:
  static void gamenodeCallback(gamenode* gn, gamenodeEvent const* event);
  void handleResponse(long msgId, JSON_Value* value);
  void handleMethodCall(long msgId, std::string const& methodName, JSON_Value* params);

  gamenode* _gn;
//  std::map<long int, MethodCallback> _callbacks;
  std::map<std::string, Method> _methods;
/*  ConnectCallback _connectCallback;
  DisconnectCallback _disconnectCallback;
  ErrorCallback _errorCallback;*/
  std::map<long, Promise<JSONValue>> _callbacks;
//  std::map<std::string, Stream<JSONValue>> _methods;
  Stream<void> _connected;
  Stream<void> _disconnected;
  Stream<void> _error;
};

Gamenode::Gamenode() : _gn(gamenodeNew(gamenodeCallback)), _callbacks()
{
  gamenodeSetUserData(_gn, this);
}

Gamenode::~Gamenode()
{
  gamenodeFree(_gn);
}

Stream<void> Gamenode::connected()
{
  return _connected;
}

Stream<void> Gamenode::disconnected()
{
  return _disconnected;
}

Stream<void> Gamenode::error()
{
  return _error;
}
/*
Stream<JSONValue> Gamenode::method(std::string const& methodName)
{
  auto iter = _methods.find(methodName);
  if(iter != _methods.end())
  {
    return iter->second;
  }
  else
  {
    Stream<JSONValue> stream;
    _methods[methodName] = stream;

    std::vector<const char*> methodList;
    for(auto& pair : _methods)
    {
      methodList.push_back(pair.first.data());
    }
    gamenodeSetMethodNames(_gn, methodList.data(), methodList.size());

    return stream;
  }
}
*/
/*
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
*/
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

Promise<JSONValue> Gamenode::call(std::string const& methodName, JSONValue const& params)
{
  JSONValue paramsClone = params;
  long msgId = gamenodeMethodCall(_gn, methodName.data(), paramsClone.extract());
  Promise<JSONValue> promise;
  _callbacks[msgId] = promise;
  return promise;
}

void Gamenode::handleResponse(long msgId, JSON_Value* value)
{
  auto iter = _callbacks.find(msgId);
  if(iter != _callbacks.end())
  {
    Promise<JSONValue> promise = iter->second;
    JSONValue v(JSON_Value_Clone(value));
    promise.fulfill(v);
    _callbacks.erase(iter);
  }
}
/*
void Gamenode::handleMethodCall(long msgId, std::string const& methodName, JSON_Value* params)
{
  auto iter = _methods.find(methodName);
  if(iter != _methods.end())
  {
    Stream<JSONValue> stream = iter->second;
    JSONValue p(JSON_Value_Clone(params));
    stream.push(p);
    //JSONValue ret = callback(p);
    //gamenodeResponse(_gn, msgId, ret.extract());
  }
}*/
/*
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
*/
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
      gnpp->_connected.push();
      break;
    }
    case GAMENODE_DISCONNECTED: break;
    {
      gnpp->_disconnected.push();
      break;
    }
    case GAMENODE_ERROR: break;
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

#endif // GAMENODEPP_H
