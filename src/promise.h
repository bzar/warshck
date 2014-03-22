#ifndef PROMISE_H
#define PROMISE_H

#include <functional>
#include <vector>
#include <memory>

template<typename T>
class Promise;
template<>
class Promise<void>;

namespace
{

  template<typename TT, typename RR>
  Promise<RR> _then(Promise<TT>& p, std::function<Promise<RR>(TT const&)> callback);

  template<typename TT>
  Promise<void> _then(Promise<TT>& p, std::function<void(TT const&)> callback);

  template<typename RR>
  Promise<RR> _then(Promise<void>& p, std::function<Promise<RR>()> callback);

  Promise<void> _then(Promise<void>& p, std::function<void()> callback);
}

template<typename T>
class Promise
{
public:
  Promise() : _callbacks(std::make_shared<Callbacks>()), _forwards(std::make_shared<Forwards>()), _result()
  {

  }

  Promise(Promise const& other) : _callbacks(other._callbacks), _forwards(other._forwards), _result(other._result)
  {

  }

  ~Promise()
  {

  }

  Promise& operator=(Promise const& other)
  {
    if(this != &other)
    {
      _callbacks = other._callbacks;
      _result = other._result;
      _forwards = other._forwards;
    }
    return *this;
  }

  template<typename R>
  Promise<R> then(std::function<Promise<R>(T const&)> callback)
  {
    return ::_then<T, R>(*this, callback);
  }

  template<typename R>
  Promise<R> then(std::function<R(T const&)> callback)
  {
    return ::_then<T>(*this, callback);
  }


  void forward(Promise<T> other)
  {
    _forwards->push_back(other);
  }

  bool fulfill(T const& t)
  {
    if(_result)
      return false;

    _result = std::make_shared<T const>(t);
    for(auto& cb : *_callbacks)
    {
      cb(t);
    }

    for(Promise<T>& fwd : *_forwards)
    {
      fwd.fulfill(t);
    }

    _callbacks->clear();
    _forwards->clear();

    return true;
  }
  /*private:
  template<typename TT, typename RR>
  friend Promise<RR> ::_then(Promise<TT>& p, std::function<RR(TT const&)> callback);

  template<typename TT>
  friend Promise<void> ::_then(Promise<TT>& p, std::function<void(TT const&)> callback);
*/
  typedef std::function<void(T const&)> Callback;
  typedef std::vector<Callback> Callbacks;
  typedef std::vector<Promise<T>> Forwards;

  std::shared_ptr<Callbacks> _callbacks;
  std::shared_ptr<T const> _result;
  std::shared_ptr<Forwards> _forwards;
};


template<>
class Promise<void>
{
public:
  Promise() : _callbacks(std::make_shared<Callbacks>()), _forwards(std::make_shared<Forwards>()), _fulfilled(false)
  {

  }

  Promise(Promise const& other) : _callbacks(other._callbacks), _forwards(other._forwards), _fulfilled(other._fulfilled)
  {

  }

  ~Promise()
  {

  }

  Promise& operator=(Promise const& other)
  {
    if(this != &other)
    {
      _callbacks = other._callbacks;
      _fulfilled = other._fulfilled;
      _forwards = other._forwards;
    }
    return *this;
  }

  template<typename R>
  Promise<R> then(std::function<Promise<R>()> callback)
  {
    return ::_then<R>(*this, callback);
  }

  template<typename R>
  Promise<R> then(std::function<R()> callback)
  {
    return ::_then(*this, callback);
  }

  void forward(Promise<void> other)
  {
    _forwards->push_back(other);
  }
  bool fulfill()
  {
    if(_fulfilled)
      return false;

    _fulfilled = true;

    for(auto& cb : *_callbacks)
    {
      cb();
    }

    for(Promise<void>& fwd : *_forwards)
    {
      fwd.fulfill();
    }

    _callbacks->clear();
    _forwards->clear();

    return true;
  }
  /*private:
  template<typename RR>
  friend Promise<RR> ::_then(Promise<void>& p, std::function<RR()> callback);

  template<>
  friend Promise<void> ::_then(Promise<void>& p, std::function<void()> callback);
*/
  typedef std::vector<std::function<void()>> Callbacks;
  typedef std::vector<Promise<void>> Forwards;
  std::shared_ptr<Callbacks> _callbacks;
  std::shared_ptr<Forwards> _forwards;

  bool _fulfilled;
};

namespace
{

  template<typename TT, typename RR>
  Promise<RR> _then(Promise<TT>& p, std::function<Promise<RR>(TT const&)> callback)
  {
    Promise<RR> promise;
    if(p._result)
    {
      promise = callback(*p._result);
    }
    else
    {
      p._callbacks->push_back([callback, promise](TT const& t) mutable {
        callback(t).forward(promise);
      });
    }

    return promise;
  }

  template<typename TT>
  Promise<void> _then(Promise<TT>& p, std::function<void(TT const&)> callback)
  {
    Promise<void> promise;
    if(p._result)
    {
      callback(*p._result);
      promise.fulfill();
    }
    else
    {
      p._callbacks->push_back([callback, promise](TT const& t) mutable {
        callback(t);
        promise.fulfill();
      });
    }

    return promise;
  }

  template<typename RR>
  Promise<RR> _then(Promise<void>& p, std::function<Promise<RR>()> callback)
  {
    Promise<RR> promise;
    if(p._fulfilled)
    {
      promise = callback();
    }
    else
    {
      p._callbacks->push_back([callback, promise]() mutable {
        callback().forward(promise);
      });
    }
    return promise;
  }

  Promise<void> _then(Promise<void>& p, std::function<void()> callback)
  {
    Promise<void> promise;
    if(p._fulfilled)
    {
      callback();
      promise.fulfill();
    }
    else
    {
      p._callbacks->push_back([callback, promise]() mutable {
        callback();
        promise.fulfill();
      });
    }

    return promise;
  }
}
#endif // PROMISE_H
