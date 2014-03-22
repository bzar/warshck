#ifndef STREAM_H
#define STREAM_H

#include <functional>
#include <vector>
#include <memory>

template<typename T>
class Stream;
template<>
class Stream<void>;

namespace
{

  template<typename TT, typename RR>
  Stream<RR> _on(Stream<TT>& p, std::function<Stream<RR>(TT const&)> callback);

  template<typename TT>
  Stream<void> _on(Stream<TT>& p, std::function<void(TT const&)> callback);

  template<typename RR>
  Stream<RR> _on(Stream<void>& p, std::function<Stream<RR>()> callback);

  Stream<void> _on(Stream<void>& p, std::function<void()> callback);
}

template<typename T>
class Stream
{
public:
  Stream() : _callbacks(std::make_shared<Callbacks>()), _forwards(std::make_shared<Forwards>())
  {

  }

  Stream(Stream const& other) : _callbacks(other._callbacks), _forwards(other._forwards)
  {

  }

  ~Stream()
  {

  }

  Stream& operator=(Stream const& other)
  {
    if(this != &other)
    {
      _callbacks = other._callbacks;
      _forwards = other._forwards;
    }
    return *this;
  }

  template<typename R>
  Stream<R> on(std::function<Stream<R>(T const&)> callback)
  {
    return ::_on<T, R>(*this, callback);
  }

  template<typename R>
  Stream<R> on(std::function<R(T const&)> callback)
  {
    return ::_on<T>(*this, callback);
  }


  void forward(Stream<T> other)
  {
    _forwards->push_back(other);
  }

  bool push(T const& t)
  {
    for(auto& cb : *_callbacks)
    {
      cb(t);
    }

    for(Stream<T>& fwd : *_forwards)
    {
      fwd.push(t);
    }

    return true;
  }
  /*private:
  template<typename TT, typename RR>
  friend Stream<RR> ::_on(Stream<TT>& p, std::function<RR(TT const&)> callback);

  template<typename TT>
  friend Stream<void> ::_on(Stream<TT>& p, std::function<void(TT const&)> callback);
*/
  typedef std::function<void(T const&)> Callback;
  typedef std::vector<Callback> Callbacks;
  typedef std::vector<Stream<T>> Forwards;

  std::shared_ptr<Callbacks> _callbacks;
  std::shared_ptr<Forwards> _forwards;
};


template<>
class Stream<void>
{
public:
  Stream() : _callbacks(std::make_shared<Callbacks>()), _forwards(std::make_shared<Forwards>())
  {

  }

  Stream(Stream const& other) : _callbacks(other._callbacks), _forwards(other._forwards)
  {

  }

  ~Stream()
  {

  }

  Stream& operator=(Stream const& other)
  {
    if(this != &other)
    {
      _callbacks = other._callbacks;
      _forwards = other._forwards;
    }
    return *this;
  }

  template<typename R>
  Stream<R> on(std::function<Stream<R>()> callback)
  {
    return ::_on<R>(*this, callback);
  }

  template<typename R>
  Stream<R> on(std::function<R()> callback)
  {
    return ::_on(*this, callback);
  }

  void forward(Stream<void> other)
  {
    _forwards->push_back(other);
  }
  bool push()
  {
    for(auto& cb : *_callbacks)
    {
      cb();
    }

    for(Stream<void>& fwd : *_forwards)
    {
      fwd.push();
    }

    return true;
  }
  /*private:
  template<typename RR>
  friend Stream<RR> ::_on(Stream<void>& p, std::function<RR()> callback);

  template<>
  friend Stream<void> ::_on(Stream<void>& p, std::function<void()> callback);
*/
  typedef std::vector<std::function<void()>> Callbacks;
  typedef std::vector<Stream<void>> Forwards;
  std::shared_ptr<Callbacks> _callbacks;
  std::shared_ptr<Forwards> _forwards;
};

namespace
{

  template<typename TT, typename RR>
  Stream<RR> _on(Stream<TT>& p, std::function<Stream<RR>(TT const&)> callback)
  {
    Stream<RR> stream;
    p._callbacks->push_back([callback, stream](TT const& t) mutable {
      callback(t).forward(stream);
    });

    return stream;
  }

  template<typename TT>
  Stream<void> _on(Stream<TT>& p, std::function<void(TT const&)> callback)
  {
    Stream<void> stream;
    p._callbacks->push_back([callback, stream](TT const& t) mutable {
      callback(t);
      stream.push();
    });

    return stream;
  }

  template<typename RR>
  Stream<RR> _on(Stream<void>& p, std::function<Stream<RR>()> callback)
  {
    Stream<RR> stream;
    p._callbacks->push_back([callback, stream]() mutable {
      callback().forward(stream);
    });
    return stream;
  }

  Stream<void> _on(Stream<void>& p, std::function<void()> callback)
  {
    Stream<void> stream;
    p._callbacks->push_back([callback, stream]() mutable {
      callback();
      stream.push();
    });

    return stream;
  }
}
#endif // STREAM_H
