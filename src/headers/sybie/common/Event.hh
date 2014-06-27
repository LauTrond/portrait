//sybie/common/Event.hh
//用于线程同步地事件类

#ifndef INCLUDE_SYBIE_COMMON_EVENT_HH
#define INCLUDE_SYBIE_COMMON_EVENT_HH

#include "sybie/common/Event_fwd.hh"

namespace sybie {
namespace common {

/* 线程安全：
 * 调用Wait()堵塞当前线程，直到另外的线程调用SetEvent()。
 * 可以有多个线程同时Wait()，一次调用SetEvent()放行全部。
 * 如果没有线程正在Wait()，调用SetEvent()不会堵塞，
 * 并会导致下一次有线程Wait()时马上通过，
 * 调用PreWait()可清空SetEvent()状态。
 */
class Event
{
public:
    Event();
    ~Event();
    Event(const Event& another) = delete;
    Event(Event&& another) throw();
    Event& operator=(const Event& another) = delete;
    Event& operator=(Event&& another) throw();
    void Swap(Event& another) throw ();
public:
    void PreWait();
    void Wait();
    void SetEvent();
private:
    void* _impl;
}; //class Event

}  //namespace common
}  //namespace sybie

#endif //ifndef INCLUDE_SYBIE_COMMON_EVENT_HH
