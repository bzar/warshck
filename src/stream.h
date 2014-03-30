#ifndef STREAM_H
#define STREAM_H

#include <functional>
#include <vector>
#include <memory>
#include <algorithm>

template<typename T>
class Stream;
template<>
class Stream<void>;

template<typename T>
class Stream
{
public:
  typedef std::function<void(T const&)> Callback;

  class Subscription
  {
  public:
    Subscription() = default;
    Subscription(Subscription const& other) = default;
    Subscription& operator=(Subscription const& other) = default;
    void unsubscribe()
    {
      _callback.reset();
    }

  private:
    friend class Stream<T>;
    Subscription(Callback callback) : _callback(std::make_shared<Callback>(callback))
    {
    }

    std::shared_ptr<Callback> _callback;
  };

  Stream() : _callbacks(std::make_shared<Callbacks>())
  {

  }

  Stream(Stream const& other) = default;
  Stream& operator=(Stream const& other) = default;

  Subscription on(Callback callback)
  {
    Subscription sub(callback);
    _callbacks->push_back({sub._callback});
    return sub;
  }

  void push(T const& t)
  {
    bool cancelledSubscriptions = false;
    for(auto& cb : *_callbacks)
    {
      auto cbs = cb.lock();
      if(cbs)
      {
        (*cbs)(t);
      }
      else
      {
        cancelledSubscriptions = true;
      }
    }

    if(cancelledSubscriptions)
    {
      _callbacks->erase(std::remove_if(_callbacks->begin(), _callbacks->end(), [](CallbackRef const& cb) {
        return !(cb.lock());
      }));
    }
  }


private:
  typedef std::weak_ptr<Callback> CallbackRef;
  typedef std::vector<CallbackRef> Callbacks;

  std::shared_ptr<Callbacks> _callbacks;
};


template<>
class Stream<void>
{
public:
  typedef std::function<void()> Callback;

  class Subscription
  {
  public:
    Subscription() = default;
    Subscription(Subscription const& other) = default;
    Subscription& operator=(Subscription const& other) = default;
    void unsubscribe()
    {
      _callback.reset();
    }

  private:
    friend class Stream<void>;
    Subscription(Callback callback) : _callback(std::make_shared<Callback>(callback))
    {
    }

    std::shared_ptr<Callback> _callback;
  };

  Stream() : _callbacks(std::make_shared<Callbacks>())
  {

  }

  Stream(Stream const& other) = default;
  Stream& operator=(Stream const& other) = default;

  Subscription on(Callback callback)
  {
    Subscription sub(callback);
    _callbacks->push_back({sub._callback});
    return sub;
  }

  void push()
  {
    bool cancelledSubscriptions = false;
    for(auto& cb : *_callbacks)
    {
      auto cbs = cb.lock();
      if(cbs)
      {
        (*cbs)();
      }
      else
      {
        cancelledSubscriptions = true;
      }
    }

    if(cancelledSubscriptions)
    {
      _callbacks->erase(std::remove_if(_callbacks->begin(), _callbacks->end(), [](CallbackRef const& cb) {
        return !(cb.lock());
      }));
    }
  }

private:
  typedef std::weak_ptr<Callback> CallbackRef;
  typedef std::vector<CallbackRef> Callbacks;
  std::shared_ptr<Callbacks> _callbacks;
};

#endif // STREAM_H
