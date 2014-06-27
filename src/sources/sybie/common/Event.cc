#include "sybie/common/Event.hh"

#include <future>
#include <mutex>

#include "sybie/common/Uncopyable.hh"

namespace sybie {
namespace common {

class EventImpl : common::Uncopyable
{
public:
    EventImpl()
        : _waiting(false), _promise(), _future(), _mutex()
    {
        PreWait();
    }

    void PreWait()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_waiting)
        {
            _waiting = true;
            _promise = std::promise<void>();
            _future = _promise.get_future();
        }
    }

    void Wait()
    {
        std::shared_future<void> future;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            future = _future;
        }
        future.wait();
        PreWait();
    }

    void SetEvent()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_waiting)
        {
            _promise.set_value();
            _waiting = false;
        }
    }

private:
    bool _waiting;
    std::promise<void> _promise;
    std::shared_future<void> _future;
    std::mutex _mutex;
}; //class EventImpl

Event::Event()
    : _impl(new EventImpl())
{
}

Event::~Event()
{
    delete (EventImpl*)_impl;
}

Event::Event(Event&& another) throw()
    : _impl(nullptr)
{
    Swap(another);
}

Event& Event::operator=(Event&& another) throw()
{
    Swap(another);
    return *this;
}

void Event::Swap(Event& another) throw()
{
    std::swap(_impl, another._impl);
}

void Event::PreWait()
{
    ((EventImpl*)_impl)->PreWait();
}

void Event::Wait()
{
    ((EventImpl*)_impl)->Wait();
}

void Event::SetEvent()
{
    ((EventImpl*)_impl)->SetEvent();
}

}  //namespace common
}  //namespace sybie
